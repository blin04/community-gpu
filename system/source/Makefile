main: main.cpp graph.cpp leader.cpp edge_betweenness.cpp modularity.cpp
	g++ -Iinclude/ main.cpp graph.cpp leader.cpp edge_betweenness.cpp modularity.cpp -lzookeeper_mt -DTHREADED -o ../bin/main
  
delete: delete.cpp
	g++ -Iinclude/ delete.cpp -lzookeeper_mt -DTHREADED -o ../bin/delete

worker: worker.cpp edge_betweenness.cpp graph.cpp modularity.cpp
	g++ -Iinclude/ worker.cpp edge_betweenness.cpp graph.cpp modularity.cpp -lzookeeper_mt -DTHREADED -o ../bin/worker

