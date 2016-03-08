# -*- coding: utf-8 -*-

# (c) Copyright 2015 Hewlett Packard Enterprise Development LP
#
# GNU Zebra is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; either version 2, or (at your option) any
# later version.
#
# GNU Zebra is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with GNU Zebra; see the file COPYING.  If not, write to the Free
# Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA.


TOPOLOGY = """
# +-------+
# |       |     +-------+
# |  hsw1  <----->  sw1  |
# |       |     +-------+
# +-------+

# Nodes
[type=openswitch name="Switch 1"] sw1
[type=host name="Host 1"] hsw1

# Links
hsw1:if01 -- sw1:if01
"""


def test_openvswitch_ct_socket_permission(topology, step):
    sw1 = topology.get("sw1")
    assert sw1 is not None
    step("1-Verify DB socket file permissions")
    # Check permissions of socket files present in /var/run/openvswitch
    output = sw1("ls -l /var/run/openvswitch", shell='bash')
    lines = output.split('\n')
    flag = True
    for line in lines:
        if 'srwxrw' in line:
            if 'ovsdb-client' not in line:
                flag = False
                break
    assert flag
