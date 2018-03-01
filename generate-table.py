#! /usr/bin/env python
from sys import stdout
pieceRotations = (
    # I
    "...." "..x." "...." ".x.."
    "xxxx" "..x." "...." ".x.."
    "...." "..x." "xxxx" ".x.."
    "...." "..x." "...." ".x..",
    # L
    "x..." ".xx." "...." ".x.."
    "xxx." ".x.." "xxx." ".x.."
    "...." ".x.." "..x." "xx.."
    "...." "...." "...." "....",
    # J
    "..x." ".x.." "...." "xx.."
    "xxx." ".x.." "xxx." ".x.."
    "...." ".xx." "x..." ".x.."
    "...." "...." "...." "....",
    # O
    ".xx." ".xx." ".xx." ".xx."
    ".xx." ".xx." ".xx." ".xx."
    "...." "...." "...." "...."
    "...." "...." "...." "....",
    # S
    ".xx." ".x.." "...." "x..."
    "xx.." ".xx." ".xx." "xx.."
    "...." "..x." "xx.." ".x.."
    "...." "...." "...." "....",
    # T
    ".x.." ".x.." "...." ".x.."
    "xxx." ".xx." "xxx." "xx.."
    "...." ".x.." ".x.." ".x.."
    "...." "...." "...." "....",
    # Z
    "xx.." "..x." "...." ".x.."
    ".xx." ".xx." "xx.." "xx.."
    "...." ".x.." ".xx." "x..."
    "...." "...." "...." "....",
)
# paste output in main.cpp
def generateTable(pieceRotations):
    indent = ' ' * 4
    for p in range(0, len(pieceRotations)):
        stdout.write(indent + "{{{")
        for r in range(0, 4):
            if r != 0:
                stdout.write(indent + " {{")
            i = 0
            for x in range(0, 4):
                for y in range(0, 4):
                    if pieceRotations[p][(16 * y) + (r * 4) + x] == 'x':
                        stdout.write("{%d, %d}" % (x, 3 - y))
                        i += 1
                        if i < 4:
                            stdout.write(", ")
            if r == 3:
                stdout.write("}")
            print("}},")

generateTable(pieceRotations)
