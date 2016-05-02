# Copyright (C) 2016 Hewlett-Packard Development Company, L.P.
# All Rights Reserved.
#

.PHONY: ops-openvswitch-test

# Add option to run OVS tests inside ops-openvswitch folder
ops-openvswitch-test:
	$(V) cd src/ops-openvswitch; ./run-ovs-tests.sh
