# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected ([<<'EOF']);
(write-bad-buff) begin
(write-bad-buff) create "test.txt"
(write-bad-buff) open "test.txt"
write-bad-buff: exit(-1)
EOF
pass;
