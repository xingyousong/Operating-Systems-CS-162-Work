# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected ([<<'EOF']);
(fd-indep) begin
(fd-indep) tell = 0
(fd-indep) tell = 0
(fd-indep) end
fd-indep: exit(0)
EOF
pass;
