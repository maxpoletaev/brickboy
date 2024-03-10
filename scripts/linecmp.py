#!/usr/bin/env python3
import os.path
from collections import deque
import argparse
import gzip
import sys
import io


class Args:
    log1: str
    log2: str
    prev: int
    lineno: bool


def find_position(line1: str, line2: str) -> str:
    positions = [' '] * max(len(line1), len(line2))
    for i, (c1, c2) in enumerate(zip(line1, line2)):
        positions[i] = '^' if c1 != c2 else ' '
    return ''.join(positions)


def parse_filename(filename: str) -> (str, int):
    skip = 0
    if ':' in filename:
        filename, skip = filename.split(':')
        skip = int(skip)
    return filename, skip


def open_file(filename: str) -> io.TextIOWrapper:
    if filename.endswith('.gz'):
        return gzip.open(filename, 'rt')
    return open(filename, 'r')


def compare(args: Args) -> bool:
    filename1, skip1 = parse_filename(args.log1)
    fp1 = open_file(filename1)
    for _ in range(skip1):
        next(fp1)

    filename2, skip2 = parse_filename(args.log2)
    fp2 = open_file(filename2)
    for _ in range(skip2):
        next(fp2)

    prev_lines = deque(maxlen=args.prev)

    for lineno, (line1, line2) in enumerate(zip(fp1, fp2), 1):
        line1 = line1.rstrip()
        line2 = line2.rstrip()

        if line1 != line2:
            for prev_line in prev_lines:
                print(prev_line)

            print()
            print(F'mismatch in line {lineno+skip1}:')
            maxlen = max(len(line1), len(line2))

            label1 = os.path.basename(filename1)
            label2 = os.path.basename(filename2)
            if label1 == label2:
                label1 = filename1
                label2 = filename2

            print(F'{line1.ljust(maxlen)} <- {label1}:{lineno+skip1}')
            print(F'{line2.ljust(maxlen)} <- {label2}:{lineno+skip2}')
            print(find_position(line1, line2))

            if args.lineno:
                print(lineno+skip2)

            return False

        prev_lines.append(line1)

    if args.lineno:
        print(0)

    return True


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('log1', help='filename:skip')
    parser.add_argument('log2', help='filename:skip')
    parser.add_argument('--prev', '-p', type=int, default=5, help='number of previous lines to show')
    parser.add_argument('--lineno', '-n', action='store_true', help='output line number at the end of the script')
    args = parser.parse_args()

    if not compare(args):
        sys.exit(1)

    print('OK')
    sys.exit(0)


if __name__ == '__main__':
    main()