#!/usr/bin/python3

import sys
import random
import math
import matplotlib.pyplot as plt
import zipf
import numpy as np
import networkx as nx
usage = '''
Usage:

python3 generate.py TYPE N DEGREE WEIGHT
where TYPE is either
- erdos
- regular
- pref
- diablo
- diablo2

N is the number of nodes
DEGREE is the expected average degree
WEIGHT is the kind of weight distribution
- uniform-X with X a float 0 < X <= 1

'''


def err(s):
    print(s)
    exit(2)


def erdos(n, deg):
    if deg < math.log(n)/2:
        err('random binomial graph should have an average degree of at least ' +
            str(math.log(n)/2)+' when n is '+str(n))

    for i in range(1000):
        G = nx.fast_gnp_random_graph(n, deg/n)
        if nx.is_connected(G):
            return G
    err('unable to generate a connected random erdos-renyi graph after 1000 iteration (expected degree is probably too low)')


def grid(n, deg):
    G = nx.grid_graph(dim=(int(math.sqrt(n)), int(math.sqrt(n))))
    return G

def cross(n, deg):
    G = nx.grid_graph(dim=(int(math.sqrt(n)), int(math.sqrt(n))))
    for node in G.nodes():
        diag1 = (node[0]+1, node[1]+1)
        diag2 = (node[0]-1, node[1]+1)

        if (G.has_node(diag1)):
            G.add_edge(node, diag1)

        if (G.has_node(diag2)):
            G.add_edge(node, diag2)        

    return G
    
# grapg random regulier
# G = nx.random_regular_graph(3, n)
def regular(n, deg):
    deg = math.floor(deg)
    if deg*n % 2 == 1:
        err('for the regular graphs, n*deg should be even')
    return nx.random_regular_graph(deg, n)


# attachement preferentiel:
# G = nx.barabasi_albert_graph(n, 2)
def pref(n, deg):
    deg = math.floor(deg)
    if deg % 2 == 1:
        err('with preferential attachment, the degree should be even')

    return nx.barabasi_albert_graph(n, deg//2)


diablo_nodeIDs = {}
diablo_nextNodeID = 0
def diabloLableToId(s:str) -> int:
    global diablo_nodeIDs
    global diablo_nextNodeID
    if s not in diablo_nodeIDs: 
        diablo_nodeIDs[s] = diablo_nextNodeID
        diablo_nextNodeID+=1
    
    return diablo_nodeIDs[s]

def diablo(n, deg):

    G = nx.Graph()
    for i in range(n):
        previousId = diabloLableToId(f'middle-{i}')
        nextId = diabloLableToId(f'middle-{i+1}')
        for j in range(n):
            betweenId = diabloLableToId(f'{i}-{j}')
            G.add_edge(previousId, betweenId)
            G.add_edge(betweenId, nextId)
    return G


def tree(n, p, level):
    #print('{} {} {} {}'.format(p, p+1, 0, 0))
    #print('{} {} {} {}'.format(p,p+2**level, 0, 0))
    #if level > 1:
    #    tree(p+1, level-1)
    #    tree(p+2**level, level-1)
    if diabloLableToId(p+'/') >= n:
        exit(0)
    print('{} {} {} {}'.format(diabloLableToId(p), diabloLableToId(p+'/'),  0, 1))
    print('{} {} {} {}'.format(diabloLableToId(p), diabloLableToId(p+'\\'), 0, 1))
    if level > 1:
        tree(n, p+'/', level-1)
        tree(n, p+'\\', level-1)

def diablo2(n, deg):
    if n < 12:
        raise Exception('for diablo 2 topology, n should be greater than 11 (here it is {})'.format(n))
    for i in range(0,11):
        print('{} {} {} {}'.format(diabloLableToId(str(i)), diabloLableToId(str(i+1)), 1, 2**i+1))
        print('{} {} {} {}'.format(diabloLableToId(str(i)), diabloLableToId(str(i+1)), 2**i/10+1, 1))
    if deg != 666:
        print('{} {} {} {}'.format(diabloLableToId(str(11)), diabloLableToId('r'), 0, 1))
    tree(n, 'r', math.log(n-11, 2))
    exit(0)
    return None

def diablo3(n, deg):
    return erdos(n, deg)


gen = None

if len(sys.argv) != 5:
    print(usage)
    exit(1)
if 'erdos' in sys.argv[1]:
    gen = erdos
elif 'regular' in sys.argv[1]:
    gen = regular
elif 'pref' in sys.argv[1]:
    gen = pref
elif 'diablo' in sys.argv[1]:
    gen = diablo
elif 'diablo2' in sys.argv[1]:
    gen = diablo2
elif 'diablo3' in sys.argv[1]:
    gen = diablo3
elif 'grid' in sys.argv[1]:
    gen = grid
elif 'cross' in sys.argv[1]:
    gen = cross

else:
    err('unknown type of graph "{}"'.format(sys.argv[1]))

n = int(sys.argv[2])
deg = float(sys.argv[3])
weightDistr, weightParam = sys.argv[4].split('-')

weightParam = float(weightParam)

if weightDistr not in ["uniform", "zipf", "diablo", "discrete", "equal"]:
    err('weight distribution must be uniform/zipf and not "{}"'.format(weightDistr))


zipfSamplesIGP = None
zipfSamplesDelay = None
if weightDistr == "zipf":
    shape = math.log(10.0) / math.log(9.0)
    zipfSamplesDelay = zipf.boundedZipf2(
        a=shape, min=10, max=weightParam*1000, size=10000)
    zipfSamplesIGP = zipf.boundedZipf2(
        a=shape, min=1, max=(2**31-1)//n//10, size=100000)

G = gen(n, deg)


def negativeIGP(delay):
    return 2**15 - delay


def randomIGP():
    if weightDistr == "uniform":
        return random.randrange(1, (2**30 - 1))//n
    elif weightDistr == "zipf":
        return zipfSamplesIGP[random.randrange(0, 10000)]
    elif weightDistr == "diablo":
        return 2**24
    elif weightDistr == "discrete":
        return random.choice(range(10,50,10))
    elif weightDistr == "equal":
        return 1
    else:
        print("Unsupported Distribution {}".format(weightDistr))


def randomDelay():
    if weightDistr == "uniform":
        return random.randrange(0, math.ceil(1000*weightParam))
    elif weightDistr == "zipf":
        return zipfSamplesDelay[random.randrange(0, 10000)]
    elif weightDistr == "diablo":
        return 0
    elif weightDistr == "discrete":
        return random.choice(range(10,50,10))
    elif weightDistr == "equal":
        return 1
    else:
        print("Unsupported Distribution {}".format(weightDistr))

if sys.argv[1] == 'diablo3':
    for i in range(0,11):
        print('{} {} {} {}'.format(i, i+1, 1, 2**i+1))
        print('{} {} {} {}'.format(i, i+1, 2**i/10+1, 1))

    print('{} {} {} {}'.format(10, 11, 1, 1))
    for a, b in G.edges:
        print('{} {} {} {}'.format(a+11, b+11, (randomDelay()/10), randomIGP()))

    exit(0)

if "grid" in sys.argv[1] or "cross" in sys.argv[1]:
    for a, b in G.edges:
        delay = randomDelay()/10
        igp = randomIGP()

        if "cross" in sys.argv[1]:
            if a[0] != b[0] and a[1] != b[1]:
                delay *= 10
                igp *= 10


        print("{} {} {} {}".format(a[0]*int(math.sqrt(n))+a[1], b[0]*int(math.sqrt(n))+b[1], delay, igp))

        if "multi" in sys.argv[1]:
            p = float(sys.argv[1].split("-")[-1])
            rand = random.randint(0,100)
            if rand < p*100:
                print("{} {} {} {}".format(a[0]*int(math.sqrt(n))+a[1], b[0]*int(math.sqrt(n))+b[1], delay, igp))

    exit(0)



for a, b in G.edges:
    print('{} {} {} {}'.format(a, b, (randomDelay()/10), randomIGP()))
    if "multi" in sys.argv[1]:
        p = float(sys.argv[1].split("-")[-1])
        rand = random.randint(0,100)
        if rand < p*100:
            print('{} {} {} {}'.format(a, b, (randomDelay()/10), randomIGP()))



