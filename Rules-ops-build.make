.PHONY ops-openvswitch-test

ops-openvswitch-test:
	cd src/ops-openvswitch && ./run-ovs-tests.sh
