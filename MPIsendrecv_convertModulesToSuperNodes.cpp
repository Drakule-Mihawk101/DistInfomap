void Network::convertModulesToSuperNodes(int tag,
		double& total_time_convertModules, int& total_iterations_convertModules,
		double& total_time_MPISendRecvConvertModules) {

	
//initialize superNodes vector for updating w.r.t. the current module status.

	vector<SuperNode>().swap(superNodes);
	superNodes.reserve(nModule);
// parallelization starting from here for now

	struct timeval startConvertModules, endConvertModules;
	struct timeval startMPI, endMPI;

	gettimeofday(&startConvertModules, NULL);

	int size, rank;
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	int numSPNodesforCurrentRank;

	vector<unsigned int> modToSPNode(nNode);// indicate corresponding superNode ID from each module.

// I am not going to parallelize the following code snippet for now, but will parallelize later
	int idx = 0;
	for (unsigned int i = 0; i < nNode; i++) {
		if (modules[i].numMembers > 0) {
			superNodes.push_back(SuperNode(modules[i], idx));
			modToSPNode[i] = idx;
			idx++;
		}
	}

	/*
	 *	Calculate outLinks and inLinks between superNodes...
	 */
	int numSPNode = superNodes.size();

	double numberOfSuperNodes = numSPNode;
	int stripSize = (numberOfSuperNodes / size) + 1;

	const unsigned int toSpNodesPackSize = nEdge + 1;
	const unsigned int totalEdges = nEdge;
	const unsigned int PackSize = 2 * stripSize;

	double linkWeightstoSpNodesSend[totalEdges];
	double linkWeightstoSpNodesReceive[totalEdges];

	int* toSpNodesSend = new int[toSpNodesPackSize]();
	int* toSpNodesReceive = new int[toSpNodesPackSize]();
	int* toSpNodesRecvAll = new int[size * toSpNodesPackSize]();

	int spNodeCountTrackerSend[PackSize];
	int spNodeCountTrackerReceive[PackSize];

	int start, end;
	findAssignedPart(&start, &end, numSPNode, size, rank);
	numSPNodesforCurrentRank = end - start;
	toSpNodesSend[nEdge] = numSPNodesforCurrentRank;

	int countSPNode = 0;

	int totalLinksCount = 0;

	for (int i = start; i < end; i++) {

		spNodeCountTrackerSend[countSPNode] = i;
		int numNodesInSPNode = superNodes[i].members.size();

		typedef map<int, double> EdgeMap;
		EdgeMap newOutLinks;
		map<int, int> NodeIndexMap;
		int noOutLinks = 0;
		int toSPNode;
		int currentSPNodeTotalLinks = 0;

		for (int j = 0; j < numNodesInSPNode; j++) {

			// Calculate newOutLinks from a superNode to other superNodes.

			Node* nd = superNodes[i].members[j];
			int nOutEdge = nd->outLinks.size();

			for (int k = 0; k < nOutEdge; k++) {
				toSPNode = modToSPNode[nodes[nd->outLinks[k].first].ModIdx()];
				if (toSPNode != i) {	// outLinks to another superNode...
					pair<map<int, int>::iterator, bool> ret =
							NodeIndexMap.insert(
									make_pair(toSPNode, totalLinksCount));
					if (ret.second) {
						toSpNodesSend[totalLinksCount] = toSPNode;
						linkWeightstoSpNodesSend[totalLinksCount] =
								nd->outLinks[k].second;
						totalLinksCount++;
						currentSPNodeTotalLinks++;
					} else {
						int spNdIndex = ret.first->second;
						linkWeightstoSpNodesSend[spNdIndex] +=
								nd->outLinks[k].second;
					}
				}
			}
		}
		spNodeCountTrackerSend[stripSize + countSPNode] =
				currentSPNodeTotalLinks;
		countSPNode++;

	}

	int totalLinks = 0;
	for (int m = 0; m < toSpNodesSend[nEdge]; m++) {
		int spNodeIndex = spNodeCountTrackerSend[m];
		int numLinks = spNodeCountTrackerSend[stripSize + m];
		superNodes[m].outLinks.reserve(numLinks);

		for (int it = totalLinks; it < totalLinks + numLinks; it++) {
			superNodes[spNodeIndex].outLinks.push_back(
					make_pair(toSpNodesSend[it], linkWeightstoSpNodesSend[it]));
		}
		totalLinks += numLinks;
	}

	/*	for (int p = 0; p < toSpNodesPackSize; p++) {
	 printf("rink:%d, stripsize:%d, toSuperNodes:%d, toSpNodesSend[%d]:%d\n",
	 rank, stripSize, numSPNode, p, toSpNodesSend[p]);
	 }

	 MPI_Allgather(toSpNodesSend, toSpNodesPackSize, MPI_INT, toSpNodesRecvAll,
	 toSpNodesPackSize, MPI_INT, MPI_COMM_WORLD);

	 for (int p = 0; p < size * toSpNodesPackSize; p++) {
	 printf("renk:%d, toSpNodesRecvAll[%d]:%d\n", rank, p,
	 toSpNodesRecvAll[p]);
	 }*/

	for (int processId = 0; processId < size; processId++) {
		if (processId != rank) {

			gettimeofday(&startMPI, NULL);
			MPI_Sendrecv(spNodeCountTrackerSend, PackSize, MPI_INT, processId,
					tag, spNodeCountTrackerReceive, PackSize,
					MPI_INT, processId, tag,
					MPI_COMM_WORLD, MPI_STATUSES_IGNORE);

			MPI_Sendrecv(toSpNodesSend, toSpNodesPackSize, MPI_INT, processId,
					tag, toSpNodesReceive, toSpNodesPackSize,
					MPI_INT, processId, tag,
					MPI_COMM_WORLD, MPI_STATUSES_IGNORE);

			MPI_Sendrecv(linkWeightstoSpNodesSend, toSpNodesPackSize,
			MPI_DOUBLE, processId, tag, linkWeightstoSpNodesReceive,
					toSpNodesPackSize, MPI_DOUBLE, processId, tag,
					MPI_COMM_WORLD, MPI_STATUSES_IGNORE);

			gettimeofday(&endMPI, NULL);

			total_time_MPISendRecvConvertModules += elapsedTimeInSec(startMPI,
					endMPI);

			int spNodeReceived = toSpNodesReceive[nEdge];

			/*int spNodeReceived = toSpNodesRecvAll[(processId + 1)
			 * toSpNodesPackSize - 1];

			 printf("reenk:%d, processId:%d, spNodeReceived:%d\n", rank,
			 processId, spNodeReceived);
			 */
			int totLinks = 0;

			for (int it = 0; it < spNodeReceived; it++) {
				int spNodeIndex = spNodeCountTrackerReceive[it];
				int numLinks = spNodeCountTrackerReceive[stripSize + it];
				superNodes[spNodeIndex].outLinks.reserve(numLinks);

				for (int iterator = totLinks; iterator < totLinks + numLinks;
						iterator++) {
					superNodes[spNodeIndex].outLinks.push_back(
							make_pair(toSpNodesReceive[iterator],
									linkWeightstoSpNodesReceive[iterator]));
				}
				totLinks += numLinks;
			}
		}
	}

// update inLinks in SEQUENTIAL..
	for (int i = 0; i < numSPNode; i++) {
		int nOutLinks = superNodes[i].outLinks.size();
		{
			for (int j = 0; j < nOutLinks; j++)
				superNodes[superNodes[i].outLinks[j].first].inLinks.push_back(
						make_pair(i, superNodes[i].outLinks[j].second));
		}
	}
	
	

	gettimeofday(&endConvertModules, NULL);

	total_time_convertModules += elapsedTimeInSec(startConvertModules,
			endConvertModules);
	total_iterations_convertModules++;

	delete[] toSpNodesSend;
	delete[] toSpNodesReceive;
	delete[] toSpNodesRecvAll;
	
	/*delete[] spNodeCountTrackerSend;
	 delete[] spNodeCountTrackerReceive;
	 delete[] linkWeightstoSpNodesSend;
	 delete[] linkWeightstoSpNodesReceive;*/

}

