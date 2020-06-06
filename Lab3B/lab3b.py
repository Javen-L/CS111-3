#!/usr/bin/env python3
# NAME: Anh Mac,William Chen
# EMAIL: anhmvc@gmail.com,billy.lj.chen@gmail.com
# UID: 905111606,405131881

import sys, csv

if len(sys.argv) != 2:
    sys.exit("Invalid arguments")

# read csv file
file = sys.argv[1]
with open(file, newline='') as csvfile:
    reader = csv.reader(csvfile, delimiter=',')
    for row in reader:
        print(','.join(row))