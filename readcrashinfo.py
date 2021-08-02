#!/usr/bin/python
import sys, binascii

def py_main(argc, argv):
    infilename = "crashinfo.dat"
    if argc >= 1:
        infilename = argv[1]
    try:
        file = open(infilename, "rb")
    except:
        print("Can't open: ", infilename)
        return
    data = bytearray(file.read())
    file.close()

    pagesize = int.from_bytes(data[0:3], "little")
    pagecount = int.from_bytes(data[4:5], "little")
    imagenumber = int.from_bytes(data[6:7], "little")
    imagesize = int.from_bytes(data[8:11], "little")
    print(pagesize, pagecount, imagenumber, imagesize)

py_main(len(sys.argv)-1, sys.argv)
