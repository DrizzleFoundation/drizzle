package GenTest::Generator::FromGrammar;

require Exporter;
@ISA = qw(GenTest GenTest::Generator);

use strict;
use GenTest::Constants;
use GenTest::Random;
use GenTest::Generator;
use GenTest::Grammar;
use GenTest::Grammar::Rule;
use GenTest;
use Cwd;

use constant GENERATOR_GRAMMAR_FILE	=> 0;
use constant GENERATOR_GRAMMAR_STRING	=> 1;
use constant GENERATOR_GRAMMAR		=> 2;
use constant GENERATOR_SEED		=> 3;
use constant GENERATOR_PRNG		=> 4;
use constant GENERATOR_TMPNAM		=> 5;
use constant GENERATOR_THREAD_ID	=> 6;
use constant GENERATOR_SEQ_ID		=> 7;
use constant GENERATOR_MASK		=> 8;
use constant GENERATOR_VARCHAR_LENGTH	=> 9;

use constant GENERATOR_MAX_OCCURRENCES	=> 500;
use constant GENERATOR_MAX_LENGTH	=> 1024;

my $field_pos;

sub new {
        my $class = shift;
	my $generator = $class->SUPER::new({
		'grammar_file'		=> GENERATOR_GRAMMAR_FILE,
		'grammar_string'	=> GENERATOR_GRAMMAR_STRING,
		'grammar'		=> GENERATOR_GRAMMAR,
		'seed'			=> GENERATOR_SEED,
		'prng'			=> GENERATOR_PRNG,
		'thread_id'		=> GENERATOR_THREAD_ID,
		'mask'			=> GENERATOR_MASK,
		'varchar_length'	=> GENERATOR_VARCHAR_LENGTH
	}, @_);

	if (not defined $generator->grammar()) {
#		say("Loading grammar file '".$generator->grammarFile()."' ...");
		$generator->[GENERATOR_GRAMMAR] = GenTest::Grammar->new(
			grammar_file	=> $generator->grammarFile(),
			grammar_string	=> $generator->grammarString()
		);
		return undef if not defined $generator->[GENERATOR_GRAMMAR];
	}

	if (not defined $generator->prng()) {
		$generator->[GENERATOR_PRNG] = GenTest::Random->new(
			seed => $generator->[GENERATOR_SEED] || 0,
			varchar_length => $generator->[GENERATOR_VARCHAR_LENGTH]
		);
	}

	$generator->[GENERATOR_SEQ_ID] = 0;

	return $generator;
}

sub prng {
	return $_[0]->[GENERATOR_PRNG];
}

sub grammar {
	return $_[0]->[GENERATOR_GRAMMAR];
}

sub grammarFile {
	return $_[0]->[GENERATOR_GRAMMAR_FILE];
}

sub grammarString {
	return $_[0]->[GENERATOR_GRAMMAR_STRING];
}

sub threadId {
	return $_[0]->[GENERATOR_THREAD_ID];
}

sub seqId {
	return $_[0]->[GENERATOR_SEQ_ID];
}

sub mask {
	return $_[0]->[GENERATOR_MASK];
}

#
# Generate a new query. We do this by iterating over the array containing grammar rules and expanding each grammar rule
# to one of its right-side components . We do that in-place in the array.
#
# Finally, we walk the array and replace all lowercase keywors with literals and such.
#

sub next {
	my ($generator, $executors) = @_;

	my $grammar = $generator->grammar();
	my $prng = $generator->prng();
	my $mask = $generator->mask();

	#
	# If a temporary file has been left from a previous statement, unlink it.
	#

	unlink($generator->[GENERATOR_TMPNAM]) if defined $generator->[GENERATOR_TMPNAM];
	$generator->[GENERATOR_TMPNAM] = undef;

	my $starting_rule;

	# If this is our first query, we look for a rule named "threadN_init" or "query_init"

	if ($generator->seqId() == 0) {
		$starting_rule = $grammar->rule("thread".$generator->threadId()."_init") || $grammar->rule("query_init");
		$mask = 0 if defined $starting_rule;						# Do not apply mask on _init rules.
	}

	# Otherwise, we look for rules named "threadN" or "query"

	$starting_rule = $grammar->rule("thread".$generator->threadId()) || $grammar->rule("query") if not defined $starting_rule;

	my @sentence;

	if ($mask > 0) {
		my $starting_sentence = $starting_rule->components();
		my @components;
		foreach my $i (0..$#$starting_sentence) {
			push @components, $starting_sentence->[$i] if ( 1 << $i ) & $mask;
		}

		if ($#components == -1) {
			say("No rule components match mask $mask. Using all components.") if $generator->seqId() == 1;
		} else {
			say("Rule components remaining after mask $mask: ".join(', ', map {$_->[0] } @components)) if $generator->seqId() == 1;
			$starting_rule = GenTest::Grammar::Rule->new(
				name => 'masked',
				components => \@components
	                );
		}
	}

	@sentence = ($starting_rule);

	my $grammar_rules = $grammar->rules();

	# And we do multiple iterations, continuously expanding grammar rules and replacing the original rule with its expansion.
	
	my %rule_counters;
	my %invariants;

	my $last_table;
	my $last_database;

	my $pos = 0;
	while ($pos <= $#sentence) {
		if ($#sentence > GENERATOR_MAX_LENGTH) {
			say("Sentence is now longer than ".GENERATOR_MAX_OCCURRENCES()." symbols. Possible endless loop in grammar. Aborting.");
			return undef;
		}
		if (ref($sentence[$pos]) eq 'GenTest::Grammar::Rule') {
			splice (@sentence, $pos, 1 , map {

				# Check if we just picked a grammar rule. If yes, then return its Rule object.	
				# If not, use the original literal, stored in $_

				if (exists $grammar_rules->{$_}) {
					$rule_counters{$_}++;
					if ($rule_counters{$_} > GENERATOR_MAX_OCCURRENCES) {
						say("Rule $_ occured more than ".GENERATOR_MAX_OCCURRENCES()." times. Possible endless loop in grammar. Aborting.");
						return undef;
					} else {
						$grammar_rules->{$_};
					}
				} else {
					$_;
				}
			} @{$prng->arrayElement($sentence[$pos]->[GenTest::Grammar::Rule::RULE_COMPONENTS])});
			return undef if $@ ne '';
		} else {
			$pos++;
		}
	}

	# Once the SQL sentence has been constructed, iterate over it to replace variable items with their final values
	
	my $item_nodash;
	my $orig_item;
	foreach (@sentence) {
		$orig_item = $_;
		next if $_ eq ' ';

		if (
			($_ =~ m{^\{}so) &&
			($_ =~ m{\}$}so)
		) {
			$_ = eval("no strict;\n".$_);		# Code

			if ($@ =~ m{at \(.*?\) line}o) {
				say("Internal grammar error: $@");
				return undef;			# Code called die()
			} elsif ($@ ne '') {
				warn("Syntax error in Perl snippet $orig_item : $@");
				return undef;
			}
			next;
		} elsif ($_ =~ m{^\$}so) {
			$_ = eval("no strict;\n".$_.";\n");	# Variable
			next;
		}

		my $modifier;

		my $invariant_substitution=0;
		if ($_ =~ m{^(_[a-z_]*?)\[(.*?)\]}sio) {
			$modifier = $2;
			if ($modifier eq 'invariant') {
				$invariant_substitution=1;
				$_ = exists $invariants{$orig_item} ? $invariants{$orig_item} : $1 ;
			} else {
				$_ = $1;
			}
		}

		next if $_ eq uc($_);				# Short-cut for UPPERCASE literals

		if ( ($_ eq 'letter') || ($_ eq '_letter') ) {
			$_ = $prng->letter();
		} elsif ($_ eq '_hex') {
			$_ = $prng->hex();
		} elsif ( ($_ eq 'digit')  || ($_ eq '_digit') ) {
			$_ = $prng->digit();
		} elsif ($_ eq '_cwd') {
			$_ = "'".cwd()."'";
		} elsif (
			($_ eq '_tmpnam') ||
			($_ eq 'tmpnam') ||
			($_ eq '_tmpfile')
		) {
			# Create a new temporary file name and record it for unlinking at the next statement
			$generator->[GENERATOR_TMPNAM] = tmpdir()."gentest".$$.".tmp" if not defined $generator->[GENERATOR_TMPNAM];
			$_ = "'".$generator->[GENERATOR_TMPNAM]."'";
			$_ =~ s{\\}{\\\\}sgio if windows();	# Backslash-escape backslashes on Windows
		} elsif ($_ eq '_tmptable') {
			$_ = "tmptable".$$;
		} elsif ($_ eq '_unix_timestamp') {
			$_ = time();
		} elsif ($_ eq '_pid') {
			$_ = $$;
		} elsif ($_ eq '_thread_count') {
			$_ = $ENV{RQG_THREADS};
		} elsif (($_ eq '_database') || ($_ eq '_db')) {
			my $databases = $executors->[0]->databases();
			$last_database = $_ = $prng->arrayElement($databases);
			$_ = '`'.$last_database.'`';
		} elsif ($_ eq '_table') {
			my $tables = $executors->[0]->tables($last_database);
			$last_table = $prng->arrayElement($tables);
			$_ = '`'.$last_table.'`';
		} elsif ($_ eq '_field') {
			my $fields = $executors->[0]->fields($last_table, $last_database);
			$_ = '`'.$prng->arrayElement($fields).'`';
		} elsif ($_ eq '_field_list') {
			my $fields = $executors->[0]->fields($last_table, $last_database);
			$_ = '`'.join('`,`', @$fields).'`';
		} elsif ($_ eq '_field_count') {
			my $fields = $executors->[0]->fields($last_table, $last_database);
			$_ = $#$fields + 1;
		} elsif ($_ eq '_field_next') {
			# Pick the next field that has not been picked recently and increment the $field_pos counter
			my $fields = $executors->[0]->fields($last_table, $last_database);
			$_ = '`'.$fields->[$field_pos++ % $#$fields].'`';
		} elsif ($_ eq '_field_no_pk') {
			my $fields = $executors->[0]->fieldsNoPK($last_table, $last_database);
			$_ = '`'.$prng->arrayElement($fields).'`';
		} elsif (($_ eq '_field_indexed') || ($_ eq '_field_key')) {
			my $fields_indexed = $executors->[0]->fieldsIndexed($last_table, $last_database);
			$_ = '`'.$prng->arrayElement($fields_indexed).'`';
		} elsif ($_ eq '_collation') {
			my $collations = $executors->[0]->collations();
			$_ = '_'.$prng->arrayElement($collations);
		} elsif ($_ eq '_charset') {
			my $charsets = $executors->[0]->charsets();
			$_ = '_'.$prng->arrayElement($charsets);
		} elsif ($_ eq '_data') {
			$_ = $prng->file(cwd()."/data");
		} elsif (
			($prng->isFieldType($_) == FIELD_TYPE_NUMERIC) ||
			($prng->isFieldType($_) == FIELD_TYPE_BLOB) 
		) {
			$_ = $prng->fieldType($_);
		} elsif ($prng->isFieldType($_)) {
			$_ = $prng->fieldType($_);
			if (($orig_item =~ m{`$}so) || ($_ =~ m{^(b'|0x)}so)) {
				# Do not quote, quotes are already present
			} elsif ($_ =~ m{'}so) {
				$_ = '"'.$_.'"';
			} else {
				$_ = "'".$_."'";
			}
		} elsif ($_ =~ m{^_(.*)}sio) {
			$item_nodash = $1;
			if ($prng->isFieldType($item_nodash)) {
				$_ = "'".$prng->fieldType($item_nodash)."'";
				if ($_ =~ m{'}so) {
					$_ = '"'.$_.'"';
				} else {
					$_ = "'".$_."'";
				}
			}
		}

		# If the grammar initially contained a ` , restore it. This allows
		# The generation of constructs such as `table _digit` => `table 5`

		if (
			($orig_item =~ m{`$}so) && 
			($_ !~ m{`}so)
		) {
			$_ = $_.'`';
		}
	
		$invariants{$orig_item} = $_ if $modifier eq 'invariant';
	}

	$generator->[GENERATOR_SEQ_ID]++;

	my $sentence = join ('', @sentence);

	# If this is a BEGIN ... END block then send it to server without splitting.
	# Otherwise, split it into individual statements so that the error and the result set from each statement
	# can be examined

	if (
		($sentence =~ m{CREATE}sio) && 
		($sentence =~ m{BEGIN|END}sio)
	) {
		return [ $sentence ];
	} elsif ($sentence =~ m{;}) {
		my @sentences = split (';', $sentence);
		return \@sentences;
	} else {
		return [ $sentence ];
	}
}

1;
