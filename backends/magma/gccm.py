###################################################################################
#  -- MAGMA (version 2.0) --
#     Univ. of Tennessee, Knoxville
#     @date
#
# File gccm.py
#
# This is a MAGMA DSL compiler. 
# Given a file, e.g., 'src.c' that contains MAGMA DSL constructs, executing
#    python gccm.py src.c
# will generate the equivalent C code in src_tmp.c that calls CUDA kernels, 
# generated in file src_tmp.cu. The original src.c file remains unchanged.
#
###################################################################################        
import sys

def main():
    # The source file is the 1st argument to the script
    if len(sys.argv) != 2:
        print('usage: %s <src.c>' % sys.argv[0])
        sys.exit(1)

    # read the input file in prog
    file = open(sys.argv[1], 'r')
    prog = file.read()
    file.close()

    fname  = sys.argv[1]
    fname  = fname[:fname.find(".")]
    cfile  = open(fname + "_tmp.c" , "w")
    cufile = open(fname + "_tmp.cu", "w")

    print('%s %s -> %s %s' % (sys.argv[0],sys.argv[1], fname+"_tmp.c", fname+"_tmp.cu"))

    # Search for the magma_template keyword in the input file
    for kernel in range(prog.count("magma_template")):
        i1 = prog.index("magma_template")
        cfile.write(prog[:i1])
        prog = prog[i1+14:]
        
        bounds   = prog[prog.find("<<")+2:prog.find(">>")] # text between << >>
        args     = prog[prog.find("(")+1 : prog.find(")")] # text/argd betweer ()
        i1, i2   = find_matching(prog, "{", "}")           # text between {}
        template = prog[i1+1:i2]                           # the template in {}

        prog     = prog[i2:]
        
        lmyarg, listargs  = find_argsbound( bounds )       # parse bounds
        listargs +=  find_argslist( args )                 # parse args

        # Replace DSL call with the real generated call in the tmp.c file
        kname = "magma_template" + "_" + str(kernel) + "_" + fname
        kcall = kname + "(" + listargs + ");"
        cfile.write(kcall)

        # prepare the magma_template kernel in the .cu file
        parts = lmyarg.split(",");                         # bound name variables

        # the kernel
        kernel  = "__global__ void                                   \n" 
        kernel +=  kname+"_kernel(                                   \n"
        kernel += "    int "+parts[0]+"begin, int "+parts[0]+"end,   \n"
        kernel += "    int "+parts[1]+"begin, int "+parts[1]+"end,   \n"
        kernel += "    int "+parts[2]+"begin, int "+parts[2]+"end,   \n"
        kernel += "    " + args+ ")                                  \n"
        kernel += "{                                                 \n"
        kernel += "   int " + parts[0] + " =  blockIdx.x;            \n"
        kernel += "   int " + parts[1] + " =  blockIdx.y;            \n"
        kernel += "   int " + parts[2] + " = threadIdx.x;            \n"
        kernel += template +             "                         \n\n"

        # the interface function calling the kernel
        kernel += "extern \"C\"                                      \n"
        kernel +=  kname+"(                                          \n"
        kernel += "    int "+parts[0]+"begin, int "+parts[0]+"end,   \n"
        kernel += "    int "+parts[1]+"begin, int "+parts[1]+"end,   \n"
        kernel += "    int "+parts[2]+"begin, int "+parts[2]+"end,   \n"
        kernel += "    " + args + ")                                 \n"
        kernel += "{                                                 \n"
        kernel += "    dim3 grid("+parts[0]+"end,"+parts[1]+"end);   \n"
        kernel += "    dim3 threads("+parts[2]+"end);              \n\n"
        kernel += "   " + kname + "_kernel<<<grid,threads,0>>>(      \n"
        kernel += "    " + parts[0]+"begin, "+parts[0]+"end,          \n"
        kernel += "   " + parts[1]+"begin, "+parts[1]+"end,          \n"
        kernel += "   " + parts[2]+"begin, "+parts[2]+"end"
        kernel += "   " + find_argslist( args ) + ");                \n" 
        kernel += "}                                               \n\n"
        
        # write the current magma templates kernel
        cufile.write(kernel)

    # write until the end the rest of the original file
    cfile.write(prog)

    # close the files
    cfile.close()
    cufile.close()

########################################################################

def find_matching(line, left, right):
    i1 = line.find(left)
    num= 1
    i2 = len(line)
    for i in range(i1,i2):
        if line[i] == right:
            num -= 1
        elif line[i] == left:
            num += 1
        if num == 0:
            return i1, i
    return i1, i2

def find_argslist( args ):
    args += ")"
    arglist = ""
    numargs = args.count(",")+1
    for i in range(numargs):
        i1 = args.find(",")
        if i1 == -1:
            i1 = args.find(")")
        for j in range(i1):
            if args[i1-j] in {' ', '*', '&'}:
                arglist = arglist + "," + args[i1-j+1:i1]
                args = args[i1+1:]
                break

    return arglist

def find_argsbound( bounds ):
    args      = ""
    argsbound = ""
    numargs = bounds.count("=")
    for i in range(numargs):
        if (i != 0 ):
            argsbound += ","
            args      += ","
        i1 = bounds.find("=")
        args = args + bounds[:i1]
        argsbound = argsbound + bounds[i1+1 : bounds.find(":")] + ","
        argsbound = argsbound + bounds[bounds.find(":")+1 : bounds.find(",")]
        bounds = bounds[bounds.find(",")+1:]

    return args, argsbound

########################################################################

if __name__ == '__main__':
   main()
