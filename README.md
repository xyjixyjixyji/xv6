# 6S081-2021-labs

The labs solution of 6.S081@MIT, Fall 2021
If you have any questions about my implementation of any lab or some confusing concepts in xv6, feel free to ...
- email me: jerryji0414@outlook.com
- open an issue to ask questions


## WHERE ARE THE CODES?

The codes are in the **different branches** other than main branch.
- e.g. syscall lab code is in **syscall branch**


## TODOS

TODOS are basically some optional challenges offered in the labs.

### Lab net

- [ ] In this lab, the networking stack uses interrupts to handle ingress packet processing, but not egress packet processing. A more sophisticated strategy would be to queue egress packets in software and only provide a limited number to the NIC at any one time. You can then rely on TX interrupts to refill the transmit ring. Using this technique, it becomes possible to prioritize different types of egress traffic. **(easy)**

- [x] The provided networking code only partially supports ARP. Implement a full ARP cache and wire it in to net_tx_eth(). **(moderate)**

- [ ] The E1000 supports multiple RX and TX rings. Configure the E1000 to provide a ring pair for each core and modify your networking stack to support multiple rings. Doing so has the potential to increase the throughput that your networking stack can support as well as reduce lock contention. **(moderate)**, but difficult to test/measure

- [ ] sockrecvudp() uses a singly-linked list to find the destination socket, which is inefficient. Try using a hash table and RCU instead to increase performance. **(easy)**, but a serious implementation would difficult to test/measure

- [ ] ICMP can provide notifications of failed networking flows. Detect these notifications and propagate them as errors through the socket system call interface.
The E1000 supports several stateless hardware offloads, including checksum calculation, RSC, and GRO. Use one or more of these offloads to increase the throughput of your networking stack. **(moderate)**, but hard to test/measure

- [ ] The networking stack in this lab is susceptible to receive livelock. Using the material in lecture and the reading assignment, devise and implement a solution to fix it. **(moderate)**, but hard to test.

- [ ] Implement a UDP server for xv6. **(moderate)**

- [ ] Implement a minimal TCP stack and download a web page. **(hard)**

### updates
Feb.16 2022: ARP CACHE for lab net checked
