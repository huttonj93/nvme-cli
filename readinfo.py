#!/usr/bin/python
import sys, binascii

def py_main(argc, argv):
    infilename = "eventinfo.dat"
    if argc >= 1:
        infilename = argv[1]
    try:
        file = open(infilename, "rb")
    except:
        print("Can't open: ", infilename)
        return
    data = bytearray(file.read())
    file.close()

    records = int.from_bytes(data[0:3], "little")
    rsize = int.from_bytes(data[4:7], "little")
    rtype = int.from_bytes(data[8:11], "little")
    print(records, rsize, rtype)

py_main(len(sys.argv)-1, sys.argv)
