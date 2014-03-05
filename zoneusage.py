#!/usr/bin/env python
import sys


def print_usage():
    print >> sys.stderr, "Usage: %s <mtrace_file> <pmap_file>" % sys.argv[0]


def read_mtrace(mtrace_file):
    ftrace = open(mtrace_file, 'r')
    line = ftrace.readline()
    while line:
        if 'Memory not freed' in line:
            break
        line = ftrace.readline()
    ftrace.readline()
    ftrace.readline()
    mallocs = []
    line = ftrace.readline()
    while line:
        # Address Size Caller
        addr, size, caller = line.strip().split(None, 2)
        caller = caller[3:]     # exclude 'at '
        mallocs.append((int(addr, 16), int(size, 16), caller))
        line = ftrace.readline()
    ftrace.close()
    return mallocs


def read_pmap(pmap_file):
    fpmap = open(pmap_file, 'r')
    lines = fpmap.readlines()
    fpmap.close()
    lines = lines[2:-2]
    zones = []
    for line in lines:
        # Address Kbytes RSS Dirty Mode Mapping
        addr, kbytes, rss, dirty, mode, mapping = line.strip().split(None, 5)
        zones.append((int(addr, 16), int(kbytes)*1024, rss, dirty, mode, mapping))
    return zones


def zone_usage(mallocs, zones):
    zones_mallocs = sorted(zones + mallocs)
    rem_size = 0
    zone_mode = ''
    print "%16s,\t%12s,\t%12s,\t%12s,\t%6s,\t%s" % ('Address', 'Bytes', 'RSS', 'Dirty', 'Mode', 'Mapping')
    for record in zones_mallocs:
        if len(record) == 6:
            if rem_size > 0:
                print "%16s,\t%12d,\t%12s,\t%12s,\t%6s,\t%s" % ('[unallocated]', rem_size, '-', '-', '-', '-')
            addr, size, rss, dirty, mode, mapping = record
            rem_size = size if 'rw' in mode else 0
            zone_mode = mode
            print "%016X,\t%12d,\t%12s,\t%12s,\t%6s,\t%s" % (addr, size, rss, dirty, mode, mapping)
        if len(record) == 3:
            addr, size, caller = record
            rem_size -= size
            print "%016X,\t%12d,\t%12s,\t%12s,\t%6s,\t%s" % (addr, size, '-', '-', zone_mode, caller)


def main():
    try:
        mtrace_file, pmap_file = sys.argv[1:3]
    except ValueError:
        print_usage()
        sys.exit(1)

    zone_usage(read_mtrace(mtrace_file), read_pmap(pmap_file))


if __name__ == '__main__':
    main()
