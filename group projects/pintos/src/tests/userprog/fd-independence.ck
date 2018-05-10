# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected ([<<'EOF']);
(fd-independence) begin
(fd-independence) end
fd-independence: exit(0)
EOF
pass;
