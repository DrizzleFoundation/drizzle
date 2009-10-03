package GenTest::Simplifier::Grammar;

require Exporter;
@ISA = qw(GenTest);

use strict;
use lib 'lib';

use GenTest;
use GenTest::Grammar;
use GenTest::Grammar::Rule;

use constant SIMPLIFIER_ORACLE		=> 0;
use constant SIMPLIFIER_CACHE		=> 1;
use constant SIMPLIFIER_GRAMMAR_OBJ	=> 2;
use constant SIMPLIFIER_RULES_VISITED	=> 3;
use constant SIMPLIFIER_GRAMMAR_FLAGS	=> 4;

1;

sub new {
        my $class = shift;

	my $simplifier = $class->SUPER::new({
		'oracle'	=> SIMPLIFIER_ORACLE,
		'grammar_flags'	=> SIMPLIFIER_GRAMMAR_FLAGS
	}, @_);

	return $simplifier;
}

sub simplify {
	my ($simplifier, $initial_grammar_string) = @_;

	if (!$simplifier->oracle($initial_grammar_string)) {
		warn("Initial grammar failed oracle check.");
		return undef;
	}
	
	my $grammar_string = $initial_grammar_string;

	#
	# We perform the descend() several times, in order to compensate for
	# our imperfect tree walking algorithm combined with the probability of 
	# loops in the grammar files.
	#

	foreach my $trial (0..1) {
		$simplifier->[SIMPLIFIER_GRAMMAR_OBJ] = GenTest::Grammar->new(
			grammar_string	=> $grammar_string,
			grammar_flags	=> $simplifier->[SIMPLIFIER_GRAMMAR_FLAGS]
		);

		return undef if not defined $simplifier->[SIMPLIFIER_GRAMMAR_OBJ];

		$simplifier->[SIMPLIFIER_RULES_VISITED] = {};

		$simplifier->descend('query');

		foreach my $rule (keys %{$simplifier->[SIMPLIFIER_GRAMMAR_OBJ]->rules()}) {
			if (not exists $simplifier->[SIMPLIFIER_RULES_VISITED]->{$rule}) {
			#	say("Rule $rule is not referenced any more. Removing from grammar.");
				$simplifier->[SIMPLIFIER_GRAMMAR_OBJ]->deleteRule($rule);
			}
		}

		$grammar_string = $simplifier->[SIMPLIFIER_GRAMMAR_OBJ]->toString();
	}
	
	if (!$simplifier->oracle($grammar_string)) {
		warn("Final grammar failed oracle check.");
		return undef;
	} else {
		return $grammar_string;
	} 
}

sub descend {
	my ($simplifier, $rule) = @_;

	my $grammar_obj = $simplifier->[SIMPLIFIER_GRAMMAR_OBJ];

	my $rule_obj = $grammar_obj->rule($rule);
	return $rule if not defined $rule_obj;

	return $rule_obj if exists $simplifier->[SIMPLIFIER_RULES_VISITED]->{$rule};
	$simplifier->[SIMPLIFIER_RULES_VISITED]->{$rule}++;

	my $orig_components = $rule_obj->components();

	for (my $component_id = $#$orig_components; $component_id >= 0; $component_id--) {
		my $orig_component = $orig_components->[$component_id];

		# Remove one component and call the oracle to check if the issue is still repeatable

	 	say("Attempting to remove component ".join(' ', @$orig_component)." ...");

		splice (@$orig_components, $component_id, 1);

		if ($simplifier->oracle($grammar_obj->toString())) {
		 	say("Outcome still repeatable after removing ".join(' ', @$orig_component).". Deleting component.");
			next;
		} else {
			say("Outcome no longer repeatable after removing ".join(' ', @$orig_component).". Keeping component.");

			# Undo the change and dig deeper, into the parts of the rule component

			splice (@$orig_components, $component_id, 0, $orig_component);

			for (my $part_id = $#{$orig_components->[$component_id]}; $part_id >= 0; $part_id--) {

				my $child = $simplifier->descend($orig_components->[$component_id]->[$part_id]);

				# If the outcome of the descend() is sufficiently simple, in-line it.

				if (ref($child) eq 'GenTest::Grammar::Rule') {
					my $child_name = $child->name();
					if ($#{$child->components()} == -1) {
					#	say("Child $child_name is empty. Removing altogether.");
						splice(@{$orig_components->[$component_id]}, $part_id, 1);
					} elsif ($#{$child->components()} == 0) {
					#    	say("Child $child_name has a single component. In-lining.");
						splice(@{$orig_components->[$component_id]}, $part_id, 1, @{$child->components()->[0]});
					}
				} else {
				#	say("Got a string literal. In-lining.");
					splice(@{$orig_components->[$component_id]}, $part_id, 1, $child);
				}
			}
		}
	}

	return $rule_obj;
}

sub oracle {
	my ($simplifier, $grammar) = @_;

	my $cache = $simplifier->[SIMPLIFIER_CACHE];
	my $oracle = $simplifier->[SIMPLIFIER_ORACLE];

	$cache->{$grammar} = $oracle->($grammar) if not exists $cache->{$grammar};
	return $cache->{$grammar};
}

1;
