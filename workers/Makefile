client: client.cpp graph.cpp leader.cpp edge_betweenness.cpp
	g++ -Iinclude/ client.cpp graph.cpp leader.cpp edge_betweenness.cpp modularity.cpp -lzookeeper_mt -DTHREADED -o ./bin/client
  
delete: delete.cpp
	g++ -Iinclude/ delete.cpp -lzookeeper_mt -DTHREADED -o ./bin/delete

