import fractions
import subprocess
import os
import sys
import random
import string
import re
from termcolor import colored
import seaborn as sns
import numpy as np
import matplotlib.pyplot as plt
import pandas as pd
import click
import networkx as nx
import itertools
import time
from itertools import chain, filterfalse
import configparser
import matplotlib.ticker as ticker
import math 
import random
import matplotlib.colors as cl
import matplotlib.gridspec as gridspec
from matplotlib.gridspec import GridSpec
from typing import List
import shutil

config = configparser.ConfigParser()
config.read('config.default.ini')
config.read('config.ini') # if it exists it will load it

NB_NODES_RANDOM = 10
USE_CASES = ["LeastDelaySR", "DCLCSR", "FRRSR", "LeastDelayNoSR", "DCLCNoSR","DCLCSR-SRG"]
MODES = ["pareto", "lex"]
ENCODINGS = ["loose", "strict"]

DataPath = config['Paths']['data']

Params = ['model', 'n', 'deg', 'weight']
ParamsTopo = ['topo', 'n']

MODEL = 0
N = 1
DEG = 2
WEIGHT = 3


TOPO = 0
TOPO_N = 1


@click.group()
def main():
    """
    Simulation tool 
    """
    pass

def parseUseCases(s):
    useCases = s.split(":")
    for useCase in useCases:
        name = getUseCaseName(useCase)
        encoding = getUseCaseEncoding(useCase)
        mode = getUseCaseMode(useCase)
        if name not in USE_CASES:
            err("Use-case {} does not exist".format(name))
        if 'NoSR' not in name and "SRG" not in name:
            if encoding not in ENCODINGS:
                err("Encoding {} does not exist".format(encoding))
            if mode not in MODES:
                err("Mode {} does not exist".format(mode))
    return useCases



def pathToParams(path: str) -> list:
    elts = []
    hd = path
    while True:
        hd, tl = os.path.split(hd)
        if tl != "":
            elts.insert(0, tl)

        if hd == "" or hd == "/":
            break

    params = [None]*len(Params)
    for el in elts:
        m = re.fullmatch('([^\\-]+)\\-(.+)', el)
        if m is not None:
            # print(m.group(1), m.group(2))
            i = Params.index(m.group(1))
            params[i] = m.group(2)
    # print(params)
    return params


def getPaths(paramList: list):
    l = [str(Params[i])+'-'+str(paramList[i]) for i in range(len(Params))]
    l = os.path.join(DataPath, *l)
    if not os.path.isdir(l):
        print_warn('path {} does not exist: skipping'.format(l))
        return
    for d in os.listdir(l):
        if os.path.isdir(os.path.join(l, d)):
            yield os.path.join(l, d)


def err(s):
    print(colored('[ERROR]\t'+s, 'red'))
    exit(2)


def getPath(paramList: list, uniqueStr: str, filename='') -> str:
    l = [str(Params[i])+'-'+str(paramList[i]) for i in range(len(Params))]
    l = os.path.join(DataPath, *l)

    l = os.path.join(l, uniqueStr)

    if filename != '':
        l = os.path.join(l, filename)

    return l


def getNewPath(paramList: list) -> str:
    while True:
        uniqueStr = ''.join(random.choices(
            string.ascii_lowercase + string.digits, k=6))
        p = getPath(paramList, uniqueStr)
        if not os.path.isdir(p):
            os.makedirs(p)
            return p

def parseWeight(weight):
    return weight.split(":")


def getPathsFromArgs(model, n, degree, weight, topo_list, each_count=None):
    if topo_list is None and n is None and model is None:
        possibleTopoFiles = []
        for d in os.listdir('.'):
            if os.path.isfile(d):
                if d.endswith('.topoList'):
                    possibleTopoFiles.append(d)
        print("You did not provide enough arguments.\nDo you want to load one of the following topo files: (type the corresponding number)")
        for i, f in enumerate(possibleTopoFiles):
            print("\t{} - {}".format(str(i+1), f))
        choice = int(input().strip())
        if choice > len(possibleTopoFiles) or choice <= 0:
            print("exiting")
            exit(0)
        topo_list = possibleTopoFiles[choice - 1]

    if topo_list:
        with open(topo_list) as f:
            paths = f.readlines()
        print_info("Using provided topo list {}".format(topo_list))
        return [x.strip() for x in paths]
    
    if n is None or model is None or weight is None or degree is None:
        err("You did not provide enough arguments.\nYou should provide at least model, n, degree and weight")

    paths = []
    print_info("Listing topo based on params")

    if n is not None:
        nArr = parseN(n)

    if nArr is None:
        print("Error: parameter n is not valid: {}".format(n))
        exit(1)
            
    degArr = parseDegree(degree, model)
    weightArr = parseWeight(weight)
    params = [None]*len(Params)
    params[MODEL] = model
    params[WEIGHT] = weight
    
    for nParam in nArr:
        params[N] = nParam
        for weightParam in weightArr:
            params[WEIGHT] = weightParam
            for degree in degArr(nParam):
                params[DEG] = degree
                newPaths = list(getPaths(params))
                
                if each_count is not None:
                    if len(newPaths) < each_count:
                        for i in range(each_count - len(newPaths)):
                            newPaths.append(_generate(params)) 
                    elif len(newPaths) > each_count:
                        newPaths = newPaths[:each_count]   
                
                    for p in newPaths:
                        if not os.path.isfile(os.path.join(p,'sr.bin')):
                            _generateSR(p)

                    newPaths = list(getPaths(params))
                    if len(newPaths) > each_count:
                        newPaths = newPaths[:each_count]   
                    
                paths.extend(newPaths)
    return paths

def _generateSR(path, nodes="--id"):
    path_topo = os.path.join(path, 'topo.txt')
    path_sr = os.path.join(path, 'sr.bin')
    if not os.path.isfile(path_topo):
        err('path "{}" does not contain a topo.txt file')


    exe = os.path.join(config["Paths"]["genSR"], config["Exe"]["genSR"])
    if not os.path.exists(exe): 
        compileRequiredExe(["genSR"])


    res = subprocess.run([exe, "--topo", "--file", path_topo, nodes,
                         "--bi-dir", "--src", "0", "--save-sr-bin", path_sr], capture_output=True)
    if res.returncode != 0:
        err('The command to generate the SR graph\n\t{}\n exited with code {}'.format(
            ' '.join([exe, "--topo", "--file", path_topo, "--id",
                      "--bi-dir", "--src", "0", "--save-sr-bin", path_sr]),
            res.returncode
        ))

    print_info('generated '+ path_sr)


def _fileEmpty(p):
    return os.path.getsize(p) == 0


def _generate(params: list, withSR=False):

    p = getNewPath(params)
    path_topo = os.path.join(p, 'topo.txt')

   
    topo = open(path_topo, "w")
    if topo is None:
        err('Unable to open a file '+path_topo)

    res = subprocess.run(["python3", "generate.py", params[MODEL], str(
        params[N]), str(params[DEG]), str(params[WEIGHT])], stdout=topo)
    topo.close()
    print_info('generated '+path_topo)

    if res.returncode != 0:
        os.remove(path_topo)
        os.rmdir(p)
        err('command to generate the topo:\n\t{}\nexited with code {}. So the folder {} has been deleted'.format(
            ' '.join(["python3", "generate.py", params[MODEL], str(
                params[N]), str(params[DEG]), str(params[WEIGHT])]),
            res.returncode,
            p
        ))
    subprocess.run(["sort", "-o", path_topo, path_topo], capture_output=True)

    if withSR:
        _generateSR(p)
    return p


def runAlgRandom(paths: List[str], useCase: str, force=False, nodes="--id", resFile=None):
    ignored = 0
    performed = 0

    # note : useCase = frr
    useCaseResFile = None 
    useCaseName =  getUseCaseName(useCase) 
    algExe = os.path.join(config["Paths"][useCaseName],config["Exe"][useCaseName])

    # if "SRG" in (useCase) == "SRG":
    #     algExe = os.path.join(config["Paths"][useCase],config["Exe"][useCase])

    for p in paths:

        # Construct necessary file paths
        if resFile != None:
            if 'NoSR' in useCase:
                res = '-'.join((resFile, useCase))
            else:
                res = '-'.join((resFile, useCase))
        else:
            useCaseResFile = useCase
            res = os.path.join(p, useCaseResFile)

        topo = os.path.join(p, 'topo.txt')
        sr = os.path.join(p, 'sr.bin')
        
        # If results exist, check if execution is forced
        if os.path.isfile(res+"-DIST") and os.path.isfile(res+"-TIME"):
            if not force:
                ignored += 1
                continue
            print_info('Exp already existing but re-launch forced')

        # Construct commands
        performed += 1
        cmd = [
            algExe, 
            "--output", res+'-TIME',
            "--results", res+"-DIST",
            "--bi-dir", 
            nodes, 
            "--all-nodes={}".format(NB_NODES_RANDOM),
            
        ]
        
        # For GOFOR and noSR, add topo
        if "NoSR" in useCase or "SRG" not in useCase:
            cmd += [
                "--topo",
                "--file", topo,
            ]
        else: 
            cmd += [
                    "--file", sr,
                    "--sr"
                ]

        
        # For everything related to SR, add sr-bin
        if "NoSR" not in useCase:
            cmd += [
                "--sr-bin",
            ]

            # if SR and not SRG, add encoding stuff
            if "SRG" not in useCase:
                cmd += [
                    "--sr-conv", sr,
                    "--encoding",  getUseCaseEncoding(useCase),
                    "--mode", getUseCaseMode(useCase),
                    "--retrieve", getUseCaseStrategy(useCase)             
                ]
                        

        print_info("Running: {}".format(' '.join(cmd)))
        ret = subprocess.run(cmd, capture_output=True)
        if ret.returncode != 0:
            err("runAlgRandom: Invalid return code: {}".format(ret.returncode))
            
    print_info('{} execution(s) ended.\n{} folder(s) ignored'.format(
        performed, ignored))
    
    return cmd
    

def getResultSingleFile(fn) -> float:
    if not os.path.isfile(fn):
        return []

    f = open(fn, 'r')
    if f is None:
        err("unable to open file {}".format(f))
    totalMs = 0
    nbLine = -1
    arr_v = []
    arr_v2 = []
    arr_v3 = []
    timeIndex = 0
    for l in f.readlines():
        nbLine += 1
        if nbLine == 0:
            timeIndex = l.strip().split().index("TIME")
            continue

        ms = [int(v) for v in l.split()][timeIndex] / 1000

        # pf = [int(v) for v in l.split()][timeIndex + 1]
        # iter = [int(v) for v in l.split()][timeIndex + 2]

        totalMs += ms
        # arr_v.append([ms, pf, iter])
        arr_v.append(ms)
        print(arr_v)

    # if nbLine <= 0:
    #     raise Exception("file {} contains no results".format(fn))

    return arr_v


def print_info(s):
    print(colored('[INFO]\t'+s.replace('\n', '\n\t'), 'cyan'))


def print_warn(s):
    print(colored('[WARN]\t'+s.replace('\n', '\n\t'), 'yellow'))


def parseN(n):
    m = re.fullmatch('(\\d+):(\\d+):(\\d+)', n)
    if m is not None:
        n = range(int(m.group(1)), int(m.group(2)), int(m.group(3)))
    else:
        m = re.fullmatch('(\\d+):(\\d+)', n)
        if m is not None:
            n = range(int(m.group(1)), int(m.group(2)))
        else:
            m = re.fullmatch('(\\d+)', n)
            if m is not None:
                n = [int(m.group(1))]
            else:
                return None
    return n

def parseRange(s, reg='.+'):
    m = re.fullmatch('({}):({}):({})'.format(reg, reg, reg), s)
    if m is not None:
        return [m.group(1), m.group(2), m.group(3)]
    else:
        m = re.fullmatch('({}):({})'.format(reg, reg), s)
        if m is not None:
            return [m.group(1), m.group(2), '1']
        else:
            m = re.fullmatch('({})'.format(reg), s)
            if m is not None:
                return [m.group(1)]
            else:
                return None


def parseDegree(deg, model):
    if "grid" in model or "cross" in model:
        return lambda n: [4]
    if type(deg) is str:
        deg = parseRange(deg)
        if len(deg) == 1:
            return lambda n: [round(eval(deg[0]), 3)]
        return lambda n: [round(x, 3) for x in np.arange(eval(deg[0]), eval(deg[1]), eval(deg[2]))]

    if type(deg) is int:
        return [lambda n: deg]
    raise Exception('cannot parse degree: {}'.format(deg))


@main.command()
@click.argument('path')
def generateSR(path):
    _generateSR(path)



@main.command()
@click.argument('count', type=int)
@click.option('--model', default='erdos', help='the model of random graph')
@click.option('--n', default='100', help='the number of nodes')
@click.option('--degree', default='10', help='the expected average degree')
@click.option('--weight', default='uniform-1.0', help='the weight distribution (Zipf of Uniform)')
@click.option('--force', default=False, help='Forces re-generation of existing graph', is_flag=True)
@click.option("--withsr", default=False, help="Also generates the SR Graphs of the generated graphs", is_flag=True)
def generate(count, model, n, degree, weight, force, withsr):
    '''
        Generate COUNT graphs with the given parameters

    Example:
        python3 sim.py generate 10 --n 100:1001:100 --model erdos --degree 'math.log(n)' --weight uniform-1.0
        python3 sim.py generate 10 --n 100:1001:100 --model erdos --degree 'math.log(n)' --weight zipf-0.5
        python3 sim.py generate 10 --n 100:1001:100 --model regular --degree 3 --weight uniform-1.0

    '''
    print('running command "generate" for {} graphs, with model: {}, n: {}, deg: {}, weight: {}'.format(
        count, model, n, degree, weight))
    nArr = parseN(n)
    if n is None:
        print("Error: parameter n is not valid: {}".format(n))
        exit(0)

    degArr = parseDegree(degree, model)
    weightArr = parseWeight(weight)
    print(weight)
    params = [None]*len(Params)
    params[MODEL] = model
    topoList = []
    for nParam in nArr:
        for degree in degArr(nParam):
            for weight in weightArr:
                print(weight)
                params[WEIGHT] = weight
                params[N] = nParam
                params[DEG] = degree
                for _ in range(count):
                    topoList.append(_generate(params, withsr))
                    
    filename = paramsSlug(topoList) + ".topoList"
    topoListFile = open(filename, 'w')
    for topo in topoList:
        topoListFile.write(topo + '\n')
    topoListFile.close()


def _removeArg(alg):
    return alg.split("-")[0]


def compileRequiredExe(useCasesArr):
    for useCase in useCasesArr:
        useCaseName = getUseCaseName(useCase)
        print('compiling {}...'.format(useCaseName))
        p = subprocess.Popen(["make", "{}".format(config["Exe"][useCaseName])], cwd=config["Paths"][useCaseName])
        p.wait()
       

@main.command()
@click.argument('file', type=str)
@click.option('--use-cases', default="LeastDelay", help='uses-cases to run.')
@click.option('--force', default=False, help='forces re-run on existing graph', is_flag=True)
@click.option('--draw', default=False, help='After the execution, generate the figures', is_flag=True)
@click.option('--resfile', default=None, help='Force result file name & path', is_flag=False)
def runOnFile(file, use_cases, force, draw, resfile):
    runOnFileCmd(file, use_cases, force, draw, resfile)


def runOnFileCmd(file, use_cases="LeastDelaySR-pareto-loose", force=False, draw=False, resFile=None):
    print("test")
    topoName = file.split("/")[-1]
    resultPath = os.path.join(config['Paths']["data"], "model-real", topoName)
    topoFile = os.path.join(resultPath, "topo.txt")
    if not os.path.exists(resultPath):
        os.makedirs(resultPath)
    if not os.path.exists(topoFile):
        shutil.copyfile(file, topoFile)
    
    srFile = os.path.join(resultPath, "sr.bin")
    if not os.path.exists(srFile):
            _generateSR(resultPath)

    useCasesArr = parseUseCases(use_cases)
    compileRequiredExe(useCasesArr)
    
    cmd = ""
    for useCase in useCasesArr:
        cmd = runAlgRandom([resultPath], useCase, force, resFile=resFile)
    
    return cmd

    # if draw:
    #     _draw([resultPath], useCasesArr)

        

def countLines(file):
    with open(file) as f:
        return sum(1 for _ in f)

def maxcolumnvalue(file, column):
    with open(file) as f:
        return max(int(line.split()[column]) for line in f)


@main.command()
@click.argument('count', type=int)
@click.option('--model', default=None, help='the model of random graph')
@click.option('--n', default=None, help='the number of nodes')
@click.option('--degree', default=None, help='the expected average degree')
@click.option('--weight', default=None, help='the weight distribution')
@click.option('--use-cases', default="delay", help='uses-cases to run.')
@click.option('--force', default=False, help='forces re-run on existing graph', is_flag=True)
@click.option('--draw', default=False, help='After the execution, generate the figures', is_flag=True)
@click.option('--topo-list', default=None, help='use the given list of topo (file containing one path per line)')
def run(count, model, n, degree, weight, force, use_cases, topo_list, draw):
    '''
    Run the algorithm on topologies with the given parameters

    Example:
        python3 sim.py run --use-cases frr:delay --n 100:1001:100 --model erdos --degree 'math.log(n)' --weight uniform-1.0
        python3 sim.py run --use-cases delay --n 100:1001:100 --model grid --weight uniform-0.01

    '''
    paths = getPathsFromArgs(model=model, n=n, degree=degree, weight=weight, topo_list=topo_list, each_count=count)

    useCasesArr = parseUseCases(use_cases)

    compileRequiredExe(useCasesArr)
        
    print_info("Running Use-cases {}".format(useCasesArr))
    time.sleep(1)
    
    for useCase in useCasesArr:
        runAlgRandom(paths, useCase, force)
    # if draw:
    #     _draw(paths, useCasesArr)


def _clean(p: str, delete, fix):
    isTopoFolder = False
    if os.path.isfile(os.path.join(p, 'topo.txt')):
        if not _fileEmpty(os.path.join(p, 'topo.txt')):
            if fix:
                print_warn("fixing folder \"{}\" : topo.txt is empty")

        if not os.path.isfile(os.path.join(p, 'sr.bin')) or _fileEmpty(os.path.join(p, 'sr.bin')):
            if fix:
                print_warn(
                    'fixing folder "{}": it does not contain an SR graph'.format(p))
                _generateSR(p)
            elif delete:
                if os.path.isfile(os.path.join(p, 'samcra.res')):
                    os.remove(os.path.join(p, 'samcra.res'))
                print_warn(
                    'deleting folder "{}": it does not contain an SR graph'.format(p))
                return
            else:
                print_warn(
                    'folder "{}" does not contain an SR graph'.format(p))
        isTopoFolder = True

    foundFolder = False
    for d in os.listdir(p):
        if os.path.isdir(os.path.join(p, d)):
            foundFolder = True
            if isTopoFolder:
                print_warn(
                    'folder "{}" contains a topo and a subdirectory'.format(p))
            else:
                _clean(os.path.join(p, d), delete, fix)
    if not foundFolder and not isTopoFolder:
        if delete:
            os.rmdir(p)
            print_warn(
                'deleting folder "{}": it contains no topo and no subdirectory'.format(p))
        else:
            print_warn(
                'folder "{}" contains no topo and no subdirectory'.format(p))


@main.command()
@click.option('--fix', default=False, help='tries to fix problematic folders', is_flag=True)
@click.option('--delete', default=False, help='delete problematic folders', is_flag=True)
def clean(delete, fix):
    p = DataPath
    _clean(p, delete, fix)

    

def paramsSlug(paths):
    # get the maximum value of the first index in params
    paramsList = [pathToParams(p) for p in paths]
    max_n = max([int(x[N]) for x in paramsList])
    min_n = min([int(x[N]) for x in paramsList])
    max_deg = max([float(x[DEG]) for x in paramsList])
    min_deg = min([float(x[DEG]) for x in paramsList])


    if max_deg != min_deg:
        return '-'.join([
            paramsList[0][MODEL],
            'deg',
            str(min_deg),
            str(max_deg),
            'n',
            str(min_n),
            str(max_n),
            paramsList[0][WEIGHT]])
    else:
        return '-'.join([
            paramsList[0][MODEL],
            'n',
            str(min_n),
            str(max_n)])


def extract_lines_with_col_value(df, column, value):
    return df.loc[df[column] == value]

@main.command()
@click.option('--model', default=None, help='the model of random graph')
@click.option('--n', default=None, help='the number of nodes')
@click.option('--degree', default=None, help='the expected average degree')
@click.option('--weight', default=None, help='the weight distribution')
@click.option('--use-case', default="delay", help='uses-cases to run.')
@click.option('--data', default="DIST", help='DIST or TIME.')
@click.option('--x', default='NBSEG', help='x values')
@click.option('--y', default='DELAY', help='y values')
@click.option('--hue', default='ENCODING', help='hue')
@click.option('--src', default=None, help='source')
@click.option('--dst', default=None, help='dest')
@click.option('--topo-list', default=None, help='use the given list of topo (file containing one path per line)')
@click.option('--open', default=False, help='Open generated PDF', is_flag=True)
@click.option('--xlabel', default=None, help='x label')
@click.option('--ylabel', default=None, help='y label')
@click.option('--legend', default=None, help='legend title')
@click.option('--count', default=None, help='how many topologies to take into account')
def lineplot(model, n, degree, weight, topo_list, use_case, x, y, hue, data, src, dst, open, xlabel, ylabel, legend, count):
    '''
    Draw the graphs for the given parameters

    Example:
        python3 sim.py draw --n 100:1001:100 --model erdos --degree 'math.log(n)' --weight uniform-1.0
        python3 sim.py draw --n 100:1001:100 --model regular --degree 3 --weight uniform-1.0

    '''
    lineplotcmd(model, n, degree, weight, use_case, x, y, hue, data, src, dst, topo_list, open, xlabel, ylabel, legend, count)

    
def lineplotcmd(model, n, degree, weight, use_case, x, y, hue, data, src=None, dst=None, topo_list=None, open=False, xlabel=None, ylabel=None, legend=None, count=None):
    '''
    Draw the graphs for the given parameters

    Example:
        python3 sim.py draw --n 100:1001:100 --model erdos --degree 'math.log(n)' --weight uniform-1.0
        python3 sim.py draw --n 100:1001:100 --model regular --degree 3 --weight uniform-1.0

    '''
    paths = getPathsFromArgs(model=model, n=n, degree=degree, weight=weight, topo_list=topo_list, each_count=count)
    useCaseArr = parseUseCases(use_case)

    df = retrieve_all_files_in_df(paths, useCaseArr, suffix=data) 
    
    if (src != None) :
        df = extract_lines_with_col_value(df, "SRC", float(src))
    if (dst != None) :
        df = extract_lines_with_col_value(df, "DST", float(dst))
    df = df.reset_index()
    # set_sns_style()
    sns.despine()
    sns.set_theme(style="whitegrid", palette="colorblind", font_scale=1.5, rc= {"axes.spines.right": False, "axes.spines.top": False})
    plt.tight_layout()
    


    
    g = sns.lineplot(
        data=df,
        x=df[x],
        y=df[y]/1000,
        hue=hue,
        style=hue,
        errorbar="sd", 
        palette="colorblind", markers=True, markersize=10
    )


    g.set(xlabel = xlabel or x.capitalize(), ylabel = ylabel or y.capitalize())

    # g.set_xticks(range(0,14))


    plt.legend(title=legend or hue.capitalize()) 
    # labels=["No SR", None,"SR Graph", "GOFOR (lex)", "GOFOR (cons)"]
    # )
    plt.tight_layout()
    # sns.set_style("whitegrid")
    # sns.set(font_scale=1.5)
    filename = '-'.join(useCaseArr)

     
    plt.savefig(filename+'.pdf')


@main.command()
@click.option('--model', default=None, help='the model of random graph')
@click.option('--n', default=None, help='the number of nodes')
@click.option('--degree', default=None, help='the expected average degree')
@click.option('--weight', default=None, help='the weight distribution')
@click.option('--use-case', default="delay", help='uses-cases to run.')
@click.option('--data', default="DIST", help='DIST or TIME.')
@click.option('--x', default='NBSEG', help='x values')
@click.option('--y', default='DELAY', help='y values')
@click.option('--hue', default='ENCODING', help='hue')
@click.option('--src', default=None, help='source')
@click.option('--dst', default=None, help='dest')
@click.option('--topo-list', default=None, help='use the given list of topo (file containing one path per line)')
@click.option('--open', default=False, help='Open generated PDF', is_flag=True)
@click.option('--log', default=(False,False), help='set log scales')
@click.option('--xlabel', default=None, help='x label')
@click.option('--ylabel', default=None, help='y label')
@click.option('--legend', default=None, help='legend title')
@click.option('--count', default=None, help='how many topologies to take into account')
def histplot(model, n, degree, weight, topo_list, use_case, x, y, hue, data, src, dst, open, log, xlabel, ylabel, legend, count):
    '''
    Draw the graphs for the given parameters

    Example:
        python3 sim.py draw --n 100:1001:100 --model erdos --degree 'math.log(n)' --weight uniform-1.0
        python3 sim.py draw --n 100:1001:100 --model regular --degree 3 --weight uniform-1.0

    '''
    histplotcmd(model, n, degree, weight, use_case, x, y, hue, data, src, dst, topo_list, open, log, open, xlabel, ylabel, legend, count)

    
def histplotcmd(model, n, degree, weight, use_case, x, y, hue, data, src=None, dst=None, topo_list=None, open=False, log=(False,False), xlabel=None, ylabel=None, legend=None, count=None):
    '''
    Draw the graphs for the given parameters

    Example:
        python3 sim.py draw --n 100:1001:100 --model erdos --degree 'math.log(n)' --weight uniform-1.0
        python3 sim.py draw --n 100:1001:100 --model regular --degree 3 --weight uniform-1.0

    '''
    paths = getPathsFromArgs(model=model, n=n, degree=degree, weight=weight, topo_list=topo_list, each_count=count)
    useCaseArr = parseUseCases(use_case)

    df = retrieve_all_files_in_df(paths, useCaseArr, suffix=data) 
    
    if (src != None) :
        df = extract_lines_with_col_value(df, "SRC", float(src))
    if (dst != None) :
        df = extract_lines_with_col_value(df, "DST", float(dst))
    
    # sns.set()
    # sns.set_theme()
    # set_sns_style()
    # sns.despine()
    # sns.set_style("darkgrid")
    # sns.set(font_scale=1.5)
    # plt.tight_layout()
    df = df.reset_index()

    # sns.despine()
    sns.set_theme(style="whitegrid", palette="colorblind", font_scale=1.5, rc= {"axes.spines.right": False, 
    "axes.spines.top": False, 
    "axes.spines.left": False,
    'axes.linewidth': 2,
    'axes.grid.axis': 'y',
    "axes.edgecolor": 'black'
    
    })
    # sns.despine(offset=10)
    # plt.tight_layout()
    plt.tight_layout()
    g = sns.histplot(
        data=df,
        x=df[x],
        hue=hue,
        palette="colorblind",
        multiple="dodge", 
        discrete=True, 
        shrink=0.95 , 
        edgecolor='black'
        # rwidth=0.75
    )


    g.set(xlabel = xlabel or x.capitalize())
    #plt.legend()
    g.get_legend().set_title(legend or hue.capitalize())

    
    for bars, hatch in zip(g.containers, ['//', '']):
        for bar in bars:
            bar.set_hatch(hatch)

    #I don't know why it should be reverse
    for legend_handle, hatch in zip(g.legend_.legendHandles, ['', '//'], ):
        # update the existing legend, use twice the hatching pattern to make it denser
        legend_handle.set_hatch(hatch + hatch)

    filename = '-'.join(useCaseArr)

    plt.tight_layout()
    plt.savefig(filename+'.pdf')
    if open:
        open_file(filename+'.pdf')



# def set_sns_style():
#     sns.despine()
#     sns.set_style("whitegrid")
#     sns.set(font_scale=1.5)
#     plt.tight_layout()

def getUseCaseName(useCase):
    if "SRG" in useCase:
        return useCase
    return useCase.split('-')[0]

def getUseCaseMode(useCase):
    if "NoSR" in getUseCaseName(useCase) or "SRG" in getUseCaseName(useCase):
        return ""
    return useCase.split('-')[1]

def getUseCaseEncoding(useCase):
    print(useCase)
    if "NoSR" in getUseCaseName(useCase) or "SRG" in getUseCaseName(useCase):
        return ""
    return useCase.split('-')[2]

def getUseCaseStrategy(useCase):
    if "NoSR" in getUseCaseName(useCase) or "SRG" in getUseCaseName(useCase):
        return ""
    return useCase.split('-')[3]


def retrieve_all_files_in_df(paths, useCaseArr, suffix):
    entire_df = pd.DataFrame()
    for p in paths:
        topo_size = p.split("/")[3].split("-")[1]
        for useCase in useCaseArr:
            resFile = useCase
            file = os.path.join(p, resFile+"-"+suffix)
            df = pd.read_csv(file, delimiter=" ")
            df = df.apply(pd.to_numeric, errors='coerce')
            df["ENCODING"] = getUseCaseEncoding(useCase)
            df["USECASENAME"] = getUseCaseName(useCase)
            df["MODE"] = getUseCaseMode(useCase)
            df["STRATEGY"] = getUseCaseStrategy(useCase)
            df["USECASE"] = useCase
            df["NB_NODES"] = topo_size
            if "SRG" in useCase:
                method = "SR Graph"
            if "pareto" in useCase:
                method = "GOFOR (cons)"
            if "lex" in useCase:
                method = "GOFOR (lex)"
            if "NoSR" in useCase:
                method = "No SR"

            df["METHOD"] = method
            entire_df = pd.concat([entire_df, df])
                    
    return entire_df


def open_file(filename):
    if sys.platform == "win32":
        os.startfile(filename)
    else:
        opener = "open" if sys.platform == "darwin" else "xdg-open"
        subprocess.call([opener, filename])
 


    

if __name__ == '__main__':
    main()
