==> You can compile by simple make command:

	> make


==> How to run RelaxMap:

	[Usage] >mpirun -n <# process> ./distInfomap <seed> <network data> <# threads> <# attempts> <threshold> <vThresh> <maxIter> <outDir> <prior/normal>

The required arguments are following:

	1) process: the number of process you want to run your application with. (e.g. 1,2, 4,.., 128 etc. )
	
	2) seed: this is for random seed value for generating random sequential order of vertices during the iterative optimization process.

	3) network data: RelaxMap currently supports two different types of the input data format. 1) pajek format (.net) and 2) edge list format (.txt)
				You can find an example of each format in the data/ directory.
				You can also find more datasets from Stanford Network Analysis Project (SNAP) network data repository.
			
	4) # thread: the number of threads. Put value 1 for the number of thread 

	5) # attempts: the number of attempts 

	6) threshold: the stop condition threshold  (recommended 1e-3 or 1e-4)

	7) vThresh: the threshold value for each vertex movement (recommended 0.0)

	8) maxIter: the number of maximum iteration for each super-step. (recommended 10)

	9) outDir: the directory where the output files will be located.

	10) prior/normal flag: apply the prioritized search for efficient runs (prior) or not (normal).  (recommended prior)


e.g. to run the application with 8 process on a network named ninetriangles.net, execute the following command

	mpirun -n 8 ./distInfomap 1 ninetriangles.net 1 1 1e-3 0.0 10 outputs/ prior
