if runTime<=scaleTill:
    print "Scaling relaxation up for",
    for v in fvSol["relaxationFactors"]:
        print v,
        fvSol["relaxationFactors"][v]*=factor
    print "Maybe this will not be read by the solver at every time-step"
    fvSol.writeFile()

if runTime==end:
    print "Restoring fvSolution to its old glory"
    fvSol.restore()

print
