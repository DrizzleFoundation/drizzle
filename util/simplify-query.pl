use warnings;
use strict;
use DBI;
use lib 'lib';
use lib '../lib';

$| = 1;

use GenTest::Constants;
use GenTest::Executor::MySQL;
use GenTest::Simplifier::Test;
use GenTest::Simplifier::SQL;
use GenTest::Comparator;

#
# This script demonstrates the simplification of queries. More information is available at
#
# http://forge.mysql.com/wiki/RandomQueryGeneratorSimplification
#

my $query = "SELECT 1 FROM DUAL";

my @desired_outcomes = (
	STATUS_CONTENT_MISMATCH,
	STATUS_LENGTH_MISMATCH
);

my @dsns = (
	'dbi:mysql:host=127.0.0.1:port=19306:user=root:database=test',
	'dbi:mysql:host=127.0.0.1:port=19308:user=root:database=test'
);

# End of user-editable part

my @executors;

foreach my $dsn (@dsns) {
	my $executor = GenTest::Executor::MySQL->new( dsn => $dsn );
	my $init_status = $executor->init();
	exit ($init_status) if $init_status != STATUS_OK;
	push @executors, $executor;
}

my $simplifier = GenTest::Simplifier::SQL->new(
	oracle => sub {
		my $oracle_query = shift;
		print ".";

		my $outcome;
		my @oracle_results;

		foreach my $executor (@executors) {
			my $oracle_result = $executor->execute($oracle_query, 1);
			push @oracle_results, $oracle_result;
		}

		if ($#executors == 0) {
			$outcome = $oracle_results[0]->status();
		} else {
			$outcome = GenTest::Comparator::compare($oracle_results[0], $oracle_results[1]);
		}

		foreach my $desired_outcome (@desired_outcomes) {
			return ORACLE_ISSUE_STILL_REPEATABLE if $outcome == $desired_outcome;
		}

		return ORACLE_ISSUE_NO_LONGER_REPEATABLE;
	}
);

my $simplified_query = $simplifier->simplify($query);

print "\nSimplified query:\n$simplified_query ;\n\n";

my @simplified_results;

foreach my $executor (@executors) {
        my $simplified_result = $executor->execute($simplified_query);
        push @simplified_results, $simplified_result;
}


my $simplifier_test = GenTest::Simplifier::Test->new(
        executors => \@executors,
        queries => [ $simplified_query, $query ],
        results => [ \@simplified_results ]
);

my $test = $simplifier_test->simplify();

print "Simplified test:\n\n";
print $test;
