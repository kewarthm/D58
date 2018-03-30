"""Custom topology example

Two directly connected switches plus a host for each switch:

   host --- switch --- switch --- host

Adding the 'topos' dict with a key/value pair to generate our newly defined
topology enables one to pass in '--topo=mytopo' from the command line.
"""

from mininet.topo import Topo

class MyTopo( Topo ):
    "Simple topology example."

    def __init__( self ):
        "Create custom topo."

        # Initialize topology
        Topo.__init__( self )

        # Add hosts and switches
        host1_1 = self.addHost( 'h1_1' )
        host1_2 = self.addHost( 'h1_2' )
        host1_3 = self.addHost( 'h1_3' )
        host1_4 = self.addHost( 'h1_4' )
        host1_r1 = self.addHost( 'h1_r1')
        switch1 = self.addSwitch( 's1' )
        #switch2 = self.addSwitch( 's2' )

        # Add links
        self.addLink( host1_1, switch1 )
        self.addLink( host1_2, switch1 )
        self.addLink( host1_3, switch1 )
        self.addLink( host1_4, switch1 )
        self.addLink( host1_r1, switch1 )


topos = { 'cscd58w18_topo_5h1s': ( lambda: MyTopo() ) }
