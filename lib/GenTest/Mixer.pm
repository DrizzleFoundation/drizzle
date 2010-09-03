# Copyright (C) 2008-2009 Sun Microsystems, Inc. All rights reserved.
# Use is subject to license terms.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2 of the License.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301
# USA

package GenTest::Mixer;

require Exporter;
@ISA = qw(GenTest);

use strict;
use Data::Dumper;
use GenTest;
use GenTest::Constants;
use GenTest::Result;
use GenTest::Validator;

use constant MIXER_GENERATOR	=> 0;
use constant MIXER_EXECUTORS	=> 1;
use constant MIXER_VALIDATORS	=> 2;
use constant MIXER_FILTERS	=> 3;

my %rule_status;

1;

sub new {
	my $class = shift;

	my $mixer = $class->SUPER::new({
		'generator'	=> MIXER_GENERATOR,
		'executors'	=> MIXER_EXECUTORS,
		'validators'	=> MIXER_VALIDATORS,
		'filters'	=> MIXER_FILTERS
	}, @_);

	foreach my $executor (@{$mixer->executors()}) {
		my $init_result = $executor->init();
		return undef if $init_result > STATUS_OK;
        $executor->cacheMetaData();
	}

	my @validators = @{$mixer->validators()};
	my %validators;

	# If a Validator was specified by name, load the class and create an object.

	foreach my $i (0..$#validators) {
		my $validator = $validators[$i];
		if (ref($validator) eq '') {
			$validator = "GenTest::Validator::".$validator;
#			say("Loading Validator $validator.");
			eval "use $validator" or print $@;
			$validators[$i] = $validator->new();
		}
		$validators{ref($validators[$i])}++;
	}

	# Query every object for its prerequisies. If one is not loaded, load it and place it
	# in front of the Validators array.

	my @prerequisites;
	foreach my $validator (@validators) {
		my $prerequisites = $validator->prerequsites();
		next if not defined $prerequisites;
		foreach my $prerequisite (@$prerequisites) {
			next if exists $validators{$prerequisite};
			$prerequisite = "GenTest::Validator::".$prerequisite;
#			say("Loading Prerequisite $prerequisite, required by $validator.");
			eval "use $prerequisite" or print $@;
			push @prerequisites, $prerequisite->new();
		}
	}

	my @validators = (@prerequisites, @validators);
	$mixer->setValidators(\@validators);

	foreach my $validator (@validators) {
		return undef if not defined $validator->init($mixer->executors());
	}

	return $mixer;
}

sub next {
	my $mixer = shift;

	my $executors = $mixer->executors();
	my $filters = $mixer->filters();

	my $queries = $mixer->generator()->next($executors);
	if (not defined $queries) {
		say("Internal grammar problem. Terminating.");
		return STATUS_ENVIRONMENT_FAILURE;
	} elsif ($queries->[0] eq '') {
#		say("Your grammar generated an empty query.");
#		return STATUS_ENVIRONMENT_FAILURE;
	}

	my $max_status = STATUS_OK;

	query: foreach my $query (@$queries) {
		next if $query =~ m{^\s*$}o;

		if (defined $filters) {
			foreach my $filter (@$filters) {
				my $explain = Dumper $executors->[0]->execute("EXPLAIN $query") if $query =~ m{^\s*SELECT}sio;
				my $filter_result = $filter->filter($query." ".$explain);
				next query if $filter_result == STATUS_SKIP;
			}
		}

		my @execution_results;
		foreach my $executor (@$executors) {
			my $execution_result = $executor->execute($query);
			$max_status = $execution_result->status() if $execution_result->status() > $max_status;
			push @execution_results, $execution_result;
			
			# If one server has crashed, do not send the query to the second one in order to preserve consistency
			if ($execution_result->status() == STATUS_SERVER_CRASHED) {
				say("Server crash reported at dsn ".$executor->dsn());
				last;
			}
		}
		
		foreach my $validator (@{$mixer->validators()}) {
			my $validation_result = $validator->validate($executors, \@execution_results);
			$max_status = $validation_result if ($validation_result != STATUS_WONT_HANDLE) && ($validation_result > $max_status);
		}
	}

	#
	# Record the lowest (best) status achieved for all participating rules. The goal
	# is for all rules to generate at least some STATUS_OK queries. If not, the offending
	# rules will be reported on DESTROY.
	#

	if (rqg_debug()) {
		my $participating_rules = $mixer->generator()->participatingRules();
		foreach my $participating_rule (@$participating_rules) {
			if (
				(not exists $rule_status{$participating_rule}) ||
				($rule_status{$participating_rule} > $max_status)
			) {
				$rule_status{$participating_rule} = $max_status
			}
		}
	}

	return $max_status;
}

sub DESTROY {
	my @rule_failures;

	foreach my $rule (keys %rule_status) {
		push @rule_failures, "$rule (".status2text($rule_status{$rule}).")" if $rule_status{$rule} > STATUS_OK;
	}

	if ($#rule_failures > -1) {
		say("The following rules produced no STATUS_OK queries: ".join(', ', @rule_failures));
	}
}

sub generator {
	return $_[0]->[MIXER_GENERATOR];
}

sub executors {
	return $_[0]->[MIXER_EXECUTORS];
}

sub validators {
	return $_[0]->[MIXER_VALIDATORS];
}

sub filters {
	return $_[0]->[MIXER_FILTERS];
}

sub setValidators {
	$_[0]->[MIXER_VALIDATORS] = $_[1];
}

1;
