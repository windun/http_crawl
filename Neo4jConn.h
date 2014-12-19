#ifndef _dO_Ob_NEO4JCONN_H_
#define _dO_Ob_NEO4JCONN_H_

#include <cstring>
#include <iostream>	
#include <curl/curl.h>	
#include <json/json.h>
#include <stdlib.h>	// realloc, malloc

class Neo4jConn
{
private:
	struct MemoryStruct {
	  char *memory;
	  size_t size;
	};
							// Transactions will be built with
	Json::Value JSON_DATA;				// either JSON
	std::string QUERY = "";				// or a Cypher Query

	int transaction_id = 0;
							// Output will either be in
							// ROW format
	std::string  row_ofilename;			//  
	std::fstream row_ofile;				// - not yet supported -
	Json::Value  row_ojson;				//
							// or GRAPH format
	std::string  graph_ofilename;			// 	output file for graph json
	std::fstream graph_ofile;			//	file stream for graph_ofilename
	Json::Value  graph_ojson;			// 	json built for graph

	Json::Value search_ojson;

	std::unordered_set<std::string> node_ids;
	std::unordered_set<std::string> edge_ids;

	std::string Trim(char * cstr)
	{
		std::string buffer = "";
		int int_value;
		for (int i = 0; cstr[i] != '\0'; i++)
	  	{
			int_value = (int)cstr[i];
			if (int_value >= 32)
			{
				buffer += cstr[i];
			}
		}
		return buffer;
	}

	static size_t
	WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
	{
		size_t realsize = size * nmemb;
		struct MemoryStruct *mem = (struct MemoryStruct *)userp;

		mem->memory = (char *)realloc(mem->memory, mem->size + realsize + 1);
		if(mem->memory == NULL) {
			/* out of memory! */ 
			printf("not enough memory (realloc returned NULL)\n");
			return 0;
		}

		memcpy(&(mem->memory[mem->size]), contents, realsize);
		mem->size += realsize;
		mem->memory[mem->size] = 0;

		return realsize;
	}
public:

	Neo4jConn (std::string row_out_filename, std::string graph_out_filename)
	{
		row_ofilename = row_out_filename;
		graph_ofilename = graph_out_filename;
		graph_ojson["nodes"] = Json::Value(Json::arrayValue);
		graph_ojson["edges"] = Json::Value(Json::arrayValue);
	}

	~Neo4jConn ()
	{

	}

	static void PrintStringChars (Json::Value data)
	{
		const char * cstr = data.toStyledString().c_str();
		for (int i = 0; cstr[i] != '\0'; i++)
	  	{
			std::cout << (int)cstr[i] << "|";
		}
		std::cout << std::endl;
	}
	
	void NewTransaction ()
	{
		JSON_DATA["statements"] = Json::Value(Json::arrayValue);
	}

	void AddTransaction (std::string str_stmt, std::string format)
	{
		Json::Value statement;
		statement["statement"] = str_stmt;
		statement["resultDataContents"] = Json::Value(Json::arrayValue);
		statement["resultDataContents"].append(format);
		JSON_DATA["statements"].append(statement);
	}

	void AddTransaction (Json::Value *json)
	{
		//std::cout << "Neo4jConn sees:" << json->toStyledString() << std::endl;
		JSON_DATA["statements"].append(*json);
	}

	void AddSearchTransaction (std::string nodes_or_edges, std::string id_label_properties, std::string property, std::string value)
	{
		////////////////////////////////////////////////////////////////////
		//	
		//	NODES
		//
		// Searching for a that has id = value
		//
		if (nodes_or_edges == "nodes")
		{ 	
			if (id_label_properties == "")
			{
				QUERY = "MATCH (a) RETURN a";
			}
			else if (id_label_properties == "id")
			{
				if (value == "")
				{
					std::cerr << "[!] this query needs a value\n";
				}
				QUERY = "MATCH (a) WHERE id(a) = " + value + " RETURN a";
			}
			//
			// Searching for a:value, that is a node with 'label' that is 'value'
			//
			else if (id_label_properties == "label")
			{
				if (value == "")
				{
					std::cerr << "[!] this query needs a value\n";
				}
				QUERY = "MATCH (a:" + value + ") RETURN a";
			}
			//
			// Searching for a node that has a property similar to value
			//
			else if (id_label_properties == "properties")
			{
				QUERY = "MATCH (a) WHERE any (" + property + " IN a." + property + " WHERE " + property + " =~ \"" + value + "\") RETURN a";
			}
		}

		////////////////////////////////////////////////////////////////////
		//	
		//	EDGES
		//
		//
		// Searching for an edge with id=value
		//
		else if (nodes_or_edges == "edges")
		{
			if (id_label_properties == "")
			{
				QUERY = "MATCH ()-[r]->() RETURN r";
			}
			else if (id_label_properties == "id")
			{
				if (value == "")
				{
					std::cerr << "[!] this query needs a value\n";
				}
				QUERY = "MATCH ()-[r]->() WHERE id(r)=" + value + " RETURN a";
			}
			//
			// Searching for a:value, that is a node with 'label' that is 'value'
			//
			else if (id_label_properties == "label")
			{
				if (value == "")
				{
					std::cerr << "[!] this query needs a value\n";
				}
				QUERY = "MATCH ()-[r:" + value + "]->() RETURN a";
			}
			//
			// Searching for a node that has a property similar to value
			//
			else if (id_label_properties == "properties")
			{
				QUERY = "MATCH ()-[r]->() WHERE any (" + property + " IN r." + property + " WHERE " + property + " =~ \"" + value + "\") RETURN r";
			}
		}
		std::cout << "[+] built query: " << QUERY << std::endl;
		AddTransaction (QUERY, "graph");
	}

	std::string Post (std::string data_str, std::string url)
	{
		CURL *curl_handle;
		CURLcode res;
		struct curl_slist *headers = NULL;
	 
		struct MemoryStruct chunk;

		chunk.memory = (char *)malloc(1);  	/* will be grown as needed by the realloc above */ 
		chunk.size = 0;    			/* no data at this point */ 

		headers = curl_slist_append(headers, "Accept: application/json; charset=UTF-8");
		headers = curl_slist_append(headers, "Content-Type: application/json");
	
		//std::cout << "Sending " << data_str << std::endl;	

		curl_handle = curl_easy_init();
		if(curl_handle) {
			curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());			// url set
			curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, headers);			// send POST message
			curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, data_str.c_str());//data);//stmts.c_str());
			curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDSIZE, data_str.size());	// Will i need CURLOPT_POSTFIELDSIZE_LARGE?
			curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);	// write response to memory
			curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);		// write response memory is &chunk
		

			//curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 1L);

			res = curl_easy_perform(curl_handle);
			if(res != CURLE_OK)
			{
				std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
			}
			else
			{
				std::cout << "Received " << chunk.size << " bytes:\n";// << chunk.memory << "\n" << std::endl;
				Json::Value json_reply;
				Json::Reader reader;
				bool parse_success = reader.parse(chunk.memory, json_reply);
				//std::cout << "Response: \n" << json_reply.toStyledString() << std::endl;
				ProcessResult(json_reply);
				PrintResultJson ();
				
			}
			curl_easy_cleanup(curl_handle);	
		}
		if(chunk.memory) free(chunk.memory);
		curl_global_cleanup();
		return "";
	}

	std::string Post (Json::Value data, std::string url)
	{
		std::string data_str = data.toStyledString();
		return Post (data_str, url);
	}

	std::string PostTransactionCommit (Json::Value data)
	{
		return Post (data, "http://localhost:7474/db/data/transaction/commit");
	}

	std::string PostTransactionCommit ()
	{
		return Post (JSON_DATA, "http://localhost:7474/db/data/transaction/commit");
	}

	// This function will generate a result json
	//	- first it prints errors to stderr
	//	- then it depends on the format of the output which is
	//	required as a parameter for the Cypher HTTP endpoint.
	//	It may either be "row" or "graph" or both.
	//
	//		"row" - ProcessRow() - NOT WORKING
	//		
	//		"graph" - ProcessGraph() - This will
	//		parse through the json that Neo4j transmits 
	//		following a transaction. This builds up a 
	//		result json - graph_ojson, which will be written
	//		to some file graph_ofile, which can then be read
	//		by the webcrawler php interface.	
	void ProcessResult (Json::Value value)		
	{
		std::cout << "Response (processed): \n";
		//PrintJsonTree(value, 0);


		////////////////////////////////////////////////////////////
		// Process all the errors and make an appropriate
		// output to stderr
		Json::Value errors = value["errors"];
		for (int i = 0; i < errors.size(); i++)
		{
			std::cout << "[!] " << errors[i] << std::endl;
		}

		Json::Value results = value["results"];

	
		////////////////////////////////////////////////////////////
		// Walk through all data
		for (int r = 0; r < results.size(); r++)
		{

			////////////////////////////////////////////////////////////
			// Neo4j first describes columns for data
			if (results[r].isMember("columns"))
			{
				// Print columns
				std::cout << "[\"columns\"] ";
				Json::Value columns = results[r]["columns"];
				for (int i = 0; i < columns.size(); i++)
				{
					std::cout << columns[i].asString() << " |";
				}
				std::cout << std::endl;
			}

			if (results[r].isMember("data"))
			{
				// Print data
				Json::Value data = results[r]["data"];
				for (int i = 0; i < data.size(); i++)
				{
					////////////////////////////////////////////////////////////
					// Format output for row data
					std::cout << "[\"data\"[" << i << "]]";
					if (data[i].isMember("row"))
					{
						Json::Value row = data[i]["row"];
						ProcessRow(row);
					}
					////////////////////////////////////////////////////////////
					// Format output for graph data
					if (data[i].isMember("graph"))
					{
						Json::Value graph = data[i]["graph"];
						ProcessGraph(graph);
					}
					std::cout << std::endl;
				}
			}
		}
	}

	void ProcessRow (Json::Value row)
	{
		for (int rw = 0; rw < row.size(); rw++)
		{
			std::cout << "[\"row\"] "; 
			for (Json::ValueIterator it = row[rw].begin(); it != row[rw].end(); it++)
			{
				std::cout << "\"" << it.key().asString() << "\":";
				if (it->type() == Json::arrayValue)
				{
					std::cout << "[";
					for (int e = 0; e < it->size(); e++)
					{
						//std::cout << (*it)[e].type() << " ";
						std::cout << (*it)[e].asString() << " ";
					}
					std::cout << "]";
				}	
				else
				{
					std::cout << (*it).asString();
				}			
				std::cout << " |";									
			}		
			//std::cout << std::endl;
		}
	}

	void ProcessGraph(Json::Value graph)
	{
		Json::Value nodes = graph["nodes"];
		Json::Value relationships = graph["relationships"];
		std::cout << "[\"nodes\"] ";

		////////////////////////////////////////////////////////////
		// Walk through all of the nodes, and create
		// appropriate Sigma.js counterparts.
		for (int i = 0; i < nodes.size(); i++)
		{
			if (node_ids.count(nodes[i]["id"].asString()) == 0)
			{
				Json::Value n;	// we will create a modified node json
						// with all labels concatenated to one
				n["id"] = nodes[i]["id"];			// copy id
				//n["properties"] = nodes[i]["properties"];	// copy properties
				n["labels"] = nodes[i]["labels"];
				std::string label;					
				for (int l = 0; l < nodes[i]["labels"].size(); l++)	// copy labels
				{							// into one string
					label += (":" + nodes[i]["labels"][l].asString());
				}
				label += "\n";
				for (Json::ValueIterator it = nodes[i]["properties"].begin(); it != nodes[i]["properties"].end(); it++)
				{
					label += it.key().asString() + ":";
					if (it->type() == Json::arrayValue)
					{
						label += "[";
						for (int e = 0; e < it->size(); e++)
						{
							label += (*it)[e].asString() + " ";
						}
						label += "]";
					}	
					else
					{
						label += (*it).asString();
					}			
					label += "\n";
				}
				n["label"] = label;				// set the label				
				graph_ojson["nodes"].append(n);
				node_ids.insert(nodes[i]["id"].asString());
			}
		}

		////////////////////////////////////////////////////////////
		// Walk through all of the edges, and create
		// appropriate Sigma.js counterparts.
		std::cout << "[\"relationships\"] ";
		for (int i = 0; i < relationships.size(); i++)
		{
			if (edge_ids.count(relationships[i]["id"].asString()) == 0)
			{
				Json::Value edge;
				edge["id"] = relationships[i]["id"];
				edge["labels"] = relationships[i]["labels"];
				edge["properties"] = relationships[i]["properties"];
				edge["source"] = relationships[i]["startNode"];
				edge["target"] = relationships[i]["endNode"];
				graph_ojson["edges"].append(edge);
				edge_ids.insert(relationships[i]["id"].asString());
			}
		}
	}

	// SearchGraph - use the processed graph stored in graph_ojson to search for
	// for specific values.
	/*
	void SearchGraph (std::string nodes_or_edges, std::string id_label_properties, std::string property, std::string value)
	{
		
		// Error checking
		if(!(nodes_or_edges == "nodes" || nodes_or_edges == "edges"))
		{
			std::cerr << "[!] Neo4jConn.SearchGraph(" << nodes_or_edges << ", " << id_label_properties << ", " << property << ", " << value << ") 1st argument not valid.\n"; 
			exit(1);
		}
		if(!(id_label_properties == "id" || id_label_properties == "label" || id_label_properties == "properties" ))
		{
			std::cerr << "[!] Neo4jConn.SearchGraph(" << nodes_or_edges << ", " << id_label_properties << ", " << property << ", " << value << ") 2nd argument not valid.\n"; 
			exit(1);
		}
		if (id_label_properties == "properties") 
		{
			if (property == "")
			{
				std::cerr << "[!] Neo4jConn.SearchGraph(" << nodes_or_edges << ", " << id_label_properties << ", " << property << ", " << value << ") needs property.\n"; 
				exit(1);
			}
		}
		else
		{
			if (property != "")
			{
				std::cerr << "[!] Neo4jConn.SearchGraph(" << nodes_or_edges << ", " << id_label_properties << ", " << property << ", " << value << ") must provide \"\" as property.\n"; 
				exit(1);
			}
		}

		// The parameters should be good at this point
		Json::Value subgraph = graph_ojson[nodes_or_edges];
		search_ojson[nodes_or_edges] = Json::Value(Json::arrayValue);
		// Loop through all the nodes or edges
		for (int i = 0; i < subgraph.size(); i++)
		{
			if(id_label_properties == "id" && subgraph[i]["id"] == value)
			{
				search_ojson[nodes_or_edges].append(subgraph[i]);
			}
			else if(id_label_properties == "label")
			{
				for (int l = 0; l < subgraph[i]["labels"][l]; l++)
				{
					
				}
				search_ojson[nodes_or_edges].append(subgraph[i]);
			}
		}
	}*/

	void PrintResultJson ()
	{
		if(row_ofilename != "")
		{
			row_ofile.open(row_ofilename, std::fstream::out);
			if (!row_ofile.is_open()) 
			{
				std::cerr << "Could not open \"row\" out file.\n";
				exit(1);
			}
			row_ofile << row_ojson.toStyledString();
			row_ofile.close();
		}
		if(graph_ofilename != "")
		{
			graph_ofile.open(graph_ofilename, std::fstream::out);
			if (!graph_ofile.is_open()) 
			{
				std::cerr << "Could not open \"graph\" out file.\n";
				exit(1);
			}
			graph_ofile << graph_ojson.toStyledString();
			graph_ofile.close();
		}
	}

	// http://stackoverflow.com/questions/4800605/iterating-through-objects-in-jsoncpp
	void PrintJsonTree (Json::Value root, int depth)
	{ 
		depth++;
		if (root.size() > 0)
		{
			for (Json::ValueIterator it = root.begin(); it != root.end(); it++)
			{
				for (int d = 0; d < depth; d++)
				{
					std::cout << "   ";
				}
				PrintValue(it.key());
				std::cout << std::endl;
				PrintJsonTree(*it, depth);
			}
			return;
		}
		else
		{
			for (int d = 0; d < depth; d++)
			{
				std::cout << "   ";
			}
			PrintValue(root); 
			std::cout << std::endl;
		}
		return;
	}

	// http://stackoverflow.com/questions/4800605/iterating-through-objects-in-jsoncpp
	void PrintValue (Json::Value val)
	{
	    if( val.isString() ) {
		std::cout << val.asString();
	    } else if( val.isBool() ) {
		std::cout << val.asBool(); 
	    } else if( val.isInt() ) {
		std::cout << val.asInt(); 
	    } else if( val.isUInt() ) {
		std::cout << val.asUInt(); 
	    } else if( val.isDouble() ) {
		std::cout << val.asDouble(); 
	    }
	    else 
	    {
		printf( "unknown type=[%d]", val.type() ); 
	    }
	}
};

#endif
