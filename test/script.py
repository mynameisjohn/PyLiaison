from argparse import ArgumentParser

# This is the argparse example,
# it takes a sequence of ints and sums or finds the max
def ParseCPPArgs(liArgs):
    liArgs = liArgs[1:]
    parser = ArgumentParser(description = "deal with some args from C++")
    parser.add_argument('integers', metavar='N', type=int, nargs='+')
    parser.add_argument('--sum', dest='accumulate', action='store_const',
                        const=sum, default=max,
                        help = 'sum the integers (default: find the max)')
    args = parser.parse_args(liArgs)
    print(args.accumulate(args.integers))
