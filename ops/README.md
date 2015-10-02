OPS-SWITCHD
===========

What is ops-switchd?
--------------------
ops-switchd is the OpenSwitch switching daemon that is a modified version of Open vSwitch. ops-switchd is responsible for driving the various switch configurations from the database into the hardware.

What is the structure of the repository?
----------------------------------------
* vswitchd - contains all source files for ops-switchd daemon.
* ovsdb - contains all source files for the transactional database.
* lib - contains all library source files used by ops-switchd daemon.
* include - contains all .h files.
* ops/tests - contains all automated tests for ops-switchd.
* ops/docs - contains the documents associated with ops-switchd.

What is the license?
--------------------
Apache 2.0 license. For more details refer to [COPYING](https://git.openswitch.net/cgit/openswitch/ops-openvswitch/tree/COPYING)

What other documents are available?
-----------------------------------
For the high level design of ops-switchd, refer to [DESIGN.md](https://git.openswitch.net/cgit/openswitch/ops-openvswitch/tree/ops/DESIGN.md)

For general information about OpenSwitch project refer to http://www.openswitch.net
