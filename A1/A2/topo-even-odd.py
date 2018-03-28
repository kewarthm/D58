from mininet.topo import Topo

"""
	To run these topologies in mininet run the following command:

	sudo mn --custom topo-even-odd.py --topo evenoddtopo --test pingall

	This will create the topology and attempt to ping every set of hosts 
	We can create several diffent topologies such as MyTopo which produces
	a simple 2 host, 2 switch network

	To run MyTopo run the following command in terminal:

	sudo mn --custom topo-even-odd.py --topo mytopo --test pingall

	When adding more topologies dont forget to extend topos so you can run them
"""

class EvenOddTopo(Topo):
	"5 Hosts 2 switches"
	
	def __init__(self):
		
		Topo.__init__(self)

		h1 = self.addHost('h1')
		h2 = self.addHost('h2')
		h3 = self.addHost('h3')
		h4 = self.addHost('h4')
		h5 = self.addHost('h5')
	
		s1 = self.addSwitch('s1')
		s2 = self.addSwitch('s2')

		self.addLink(h1, s1)
		self.addLink(h2, s1)
		self.addLink(h3, s1)
		self.addLink(h4, s2)
		self.addLink(h5, s2)
		self.addLink(s1, s2)


class MyTopo(Topo):
	"2 Hosts 2 switches"
	
	def __init__(self):
		
		Topo.__init__(self)

		h1 = self.addHost('h1')
		h2 = self.addHost('h2')
	
		s1 = self.addSwitch('s1')
		s2 = self.addSwitch('s2')

		self.addLink(h1, s1)
		self.addLink(h2, s2)
		self.addLink(s1, s2)


topos = { 'evenoddtopo': ( lambda: EvenOddTopo() ), 'mytopo': ( lambda: MyTopo() )}
