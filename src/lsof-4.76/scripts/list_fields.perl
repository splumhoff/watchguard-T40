#!/usr/local/bin/perl4
#
# $Id$
#
# list_fields.perl -- sample Perl script to list lsof full field output
#		      (i.e., -F output without -0)
#
# This script has been tested under perl versions 4.036 and 5.001e.
#
# Copyright 1994 Purdue Research Foundation, West Lafayette, Indiana
# 47907.  All rights reserved.
#
# Written by Victor A. Abell
#
# This software is not subject to any license of the American Telephone
# and Telegraph Company or the Regents of the University of California.
#
# Permission is granted to anyone to use this software for any purpose on
# any computer system, and to alter it and redistribute it freely, subject
# to the following restrictions:
#
# 1. Neither the authors nor Purdue University are responsible for any
#    consequences of the use of this software.
#
# 2. The origin of this software must not be misrepresented, either by
#    explicit claim or by omission.  Credit to the authors and Purdue
#    University must appear in documentation and sources.
#
# 3. Altered versions must be plainly marked as such, and must not be
#    misrepresented as being the original software.
#
# 4. This notice may not be removed or altered.

# Initialize variables.

$fhdr = 0;							# fd hdr. flag
$fdst = 0;							# fd state
$access = $devch = $devn = $fd = $inode = $lock = $name = "";	# | file descr.
$offset = $proto = $size = $state = $stream = $type = "";	# | variables
$pidst = 0;							# process state
$cmd = $login = $pgrp = $pid = $ppid = $uid = "";		# process var.

# Process the ``lsof -F'' output a line at a time, gathering
# the variables for a process together before printing them;
# then gathering the variables for each file descriptor
# together before printing them.

while (<>) {
    chop;
    if (/^p(.*)/) {

# A process set begins with a PID field whose ID character is `p'.

	$tpid = $1;
	if ($pidst) { &list_proc }
	$pidst = 1;
	$pid = $tpid;
	if ($fdst) { &list_fd; $fdst = 0; }
	next;
    }

# Save process-related values.

    if (/^g(.*)/) { $pgrp = $1; next; }
    if (/^c(.*)/) { $cmd = $1; next; }
    if (/^u(.*)/) { $uid = $1; next; }
    if (/^L(.*)/) { $login = $1; next; }
    if (/^R(.*)/) { $ppid = $1; next; }

# A file descriptor set begins with a file descriptor field whose ID
# character is `f'.

    if (/^f(.*)/) {
	$tfd = $1;
	if ($pidst) { &list_proc }
	if ($fdst) { &list_fd }
	$fd = $tfd;
	$fdst = 1;
	next;
    }

# Save file set information.

    if (/^a(.*)/) { $access = $1; next; }
    if (/^C(.*)/) { next; }
    if (/^d(.*)/) { $devch = $1; next; }
    if (/^D(.*)/) { $devn = $1; next; }
    if (/^F(.*)/) { next; }
    if (/^G(.*)/) { next; }
    if (/^i(.*)/) { $inode = $1; next; }
    if (/^k(.*)/) { next; }
    if (/^l(.*)/) { $lock = $1; next; }
    if (/^N(.*)/) { next; }
    if (/^o(.*)/) { $offset = $1; next; }
    if (/^P(.*)/) { $proto = $1; next; }
    if (/^s(.*)/) { $size = $1; next; }
    if (/^S(.*)/) { $stream = $1; next; }
    if (/^t(.*)/) { $type = $1; next; }
    if (/^T(.*)/) {
	if ($state eq "") { $state = "(" . $1; }
	else { $state = $state . " " . $1; }
	next;
    }
    if (/^n(.*)/) { $name = $1; next; }
    print "ERROR: unrecognized: \"$_\"\n";
}

# Flush any stored file or process output.

if ($fdst) { &list_fd }
if ($pidst) { &list_proc }
exit(0);


## list_fd -- list file descriptor information
#	      Values are stored inelegantly in global variables.

sub list_fd {
    if ( ! $fhdr) {

    # Print header once.

	print "      FD   TYPE      DEVICE   SIZE/OFF      INODE  NAME\n";
	$fhdr = 1;
    }
    printf "    %4s%1.1s%1.1s %4.4s", $fd, $access, $lock, $type;
    $tmp = $devn; if ($devch ne "") { $tmp = $devch }
    printf "  %10.10s", $tmp;
    $tmp = $size; if ($offset ne "") { $tmp = $offset }
    printf " %10.10s", $tmp;
    $tmp = $inode; if ($proto ne "") { $tmp = $proto }
    printf " %10.10s", $tmp;
    $tmp = $stream; if ($name ne "") { $tmp = $name }
    print "  ", $tmp;
    if ($state ne "") { printf " %s)\n", $state; } else { print "\n"; }

# Clear variables.

    $access = $devch = $devn = $fd = $inode = $lock = $name = "";
    $offset = $proto = $size = $state = $stream = $type = "";
}


# list_proc -- list process information
#	       Values are stored inelegantly in global variables.

sub list_proc {
    print "COMMAND       PID    PGRP    PPID  USER\n";
    $tmp = $uid; if ($login ne "") {$tmp = $login }
    printf "%-9.9s  %6d  %6d  %6d  %s\n", $cmd, $pid, $pgrp, $ppid, $tmp;

# Clear variables.

    $cmd = $login = $pgrp = $pid = $uid = "";
    $fhdr = $pidst = 0;
}
