#!/usr/bin/perl
# Copyright (c) 2010 Przemysław Sitek
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

# uConfigure 1.0
# (c) Przemysław Sitek
# Simple configuration tool

use strict;
use warnings;


package uConfigure;

my $ERR = "2>>configure.err";

# Create new uConfigure object
sub new
{
	my $type = shift;
	my $self = {
		MISSING => [],        # List of missing items
		LINE_LEN => 0,        # Current line len (for fancy printing)
		DEFINES => {},        # Hash of defines
		CFLAGS => "",         # Flags for compiler
		LDFLAGS => "",        # Flags for linker
	};
	bless ($self, $type);
	return $self;
}

# Checks API

#############################################################################

# Check for command presence
sub check_cmd
{
	my $self = shift;
	my $cmd_name = shift;
	my $required = shift;
	$required = defined $required ? $required : 1;

	$self->print_check ($cmd_name);

	# Search for command in $PATH
	my $found = "";
	my @PATH = split ':', $ENV{PATH};
	foreach my $dir (@PATH) {
		my $file = "$dir/$cmd_name";
		if (-x $file && -x $file) {
			$found = $file;
			last;
		}
	}

	if ($required && $found eq "") {
		$self->add_missing ($cmd_name);
	}

	$self->print_status ($found, $found ne "");
	return $found;
}


# Check for package availability using pkg-config
sub check_pkg
{
	my $self = shift;
	my $package_name = shift;
	my $required = shift;
	$required = defined $required ? $required : 1;

	$self->print_check ($package_name);

	# Run pkg-config
	my $cmd = "pkg-config --modversion '$package_name' $ERR";
	my $version = `$cmd`;
	my $status = $?;
	chomp $version;

	my $found = $status == 0;

	# Get compiler flags
	if ($found) {
		my $cflags = `pkg-config --cflags '$package_name' $ERR`;
		chomp $cflags;
		$self->{CFLAGS} .= " $cflags";

		my $ldflags = `pkg-config --libs '$package_name' $ERR`;
		chomp $ldflags;
		$self->{LDFLAGS} .= " $ldflags";
	}

	if ($required && !$found) {
		$self->add_missing ($package_name);
	}

	$self->print_status ($version, $found);

	return $found;
}


# Check for a header file
sub check_header
{
	my $self = shift;
	my $header = shift;
	my $required = shift;
	$required = defined $required ? $required : 1;

	$self->print_check ($header);
	my $found = 0;

	# Prepare test file
	my $test_file="#include <$header>\nint main (void) { return 0; }\n";

	open F, '>', "test.c";
	print F $test_file;
	close F;

	# compile test file
	my $cmd = sprintf ("gcc -Wall %s -c -o test.o test.c $ERR",
		$self->{CFLAGS});
	my $exit_code = system ($cmd);
	unlink "test.c";
	unlink "test.o" if (-f "test.o");

	$found = $exit_code == 0;

	if ($required && !$found) {
		$self->add_missing ($header);
	}

	$self->print_status ("", $found);

	return $found;
}

# Check for a library
sub check_lib
{
	my $self = shift;
	my $lib = shift;
	my $required = shift;
	$required = defined $required ? $required : 1;

	$self->print_check ("lib$lib");
	my $found = 0;

	# Prepare test file
	my $test_file="int main (void) { return 0; }\n";

	open F, '>', "test.c";
	print F $test_file;
	close F;

	# compile test file
	my $cmd = "gcc -Wall -l$lib -o test test.c $ERR";
	my $exit_code = system ($cmd);
	unlink "test.c";
	unlink "test" if (-f "test");

	$found = $exit_code == 0;

	if ($required && !$found) {
		$self->add_missing ("lib$lib");
	}

	$self->print_status ("", $found);

	return $found;
}


# State API

#############################################################################

# Add error to list of errors
sub add_missing
{
	my $self = shift;
	my $item = shift;

	push @{$self->{MISSING}}, $item;
}

# Print summary
sub print_summary
{
	my $self = shift;

	my $n = @{$self->{MISSING}};
	if ($n > 0) {
		print "\nFollowing dependencies were not met:\n";
		foreach my $item (@{$self->{MISSING}}) {
			print " * $item\n";
		}
	} else {
		print "\nConfiguration succesful\n";
	}

#	print "CFLAGS = ", $self->{CFLAGS}, "\n";
#	print "LDFLAGS = ", $self->{LDFLAGS}, "\n";
}


# Whether configuration was succesful
sub ok
{
	my $self = shift;
	my $n = @{$self->{MISSING}};
	return $n == 0;
}


# Misc

#############################################################################

# Define a constant
sub define
{
	my $self = shift;
	my $key = shift;
	my $val = shift;

	$self->{DEFINES}->{$key} = $val;
}

# Write all definitions into a header file
sub write_defines
{
	my $self = shift;
	my $file_name = shift;

	open F, '>', $file_name;

	print F "/*\n * Generated by uConfigure\n * Please do not edit\n */\n\n";

	for my $key (keys %{$self->{DEFINES}}) {
		my $val = $self->{DEFINES}->{$key};

		if (defined $val) {
			# TODO: proper escaping
			print F "#define $key \"$val\"\n";
		} else {
			print F "#define $key\n";
		}
	}

	close F;
}

# Write file for inclusion in Makefile
sub write_makefile_inc
{
	my $self = shift;
	my $file_name = shift;

	open F, '>', $file_name;

	print F "# Generated by uConfigure\n# Please do not edit\n\n";
	print F "UC_CFLAGS=",$self->{CFLAGS},"\n";
	print F "UC_LDFLAGS=",$self->{LDFLAGS},"\n";

	close F;
}

# Private

#############################################################################

# Output
sub print_check
{
	my $self = shift;
	my $text = shift;

	my $line = "Checking for $text... ";
	$self->{LINE_LEN} = length ($line);

	print $line;
}


# Print check status (fancy)
sub print_status
{
	my $self = shift;
	my $detail = shift;
	my $ok = shift;

	my $use_colors = 1;

	my $screen_width = 80;
	my $space = $screen_width - $self->{LINE_LEN} - length ($detail) - 8;

	my $label_ok = "[ ok ]";
	my $label_fail = "[fail]";


	if ($use_colors) {
		$label_ok = "[ \033[1;32mok\033[0m ]";
		$label_fail = "[\033[1;31mfail\033[0m]";
	}

	print "$detail ";
	print " " x $space;
	print $ok ? $label_ok : $label_fail;
	print "\n";
}


1;
