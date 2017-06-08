//
//  main.cpp
//  findPatterns
//
//  Created by Taiki on 28/05/2017.
//  Copyright © 2017 Taiki. All rights reserved.
//

#include <iostream>
#include <vector>
#include <algorithm>
#include <sstream>
#include <fstream>
#include <map>

#define PATERN_BOUNDARY 0xF0000000

using namespace std;

bool parseFile(const char * fileName, vector<uint32_t> & vector);
void writeData(fstream & output, uint depth, map<uint32_t, pair<uint32_t, vector<uint32_t>>> & paterns, vector<uint32_t> & data);
bool fillCursorStartingAt(vector<uint32_t>::const_iterator & data, const vector<uint32_t>::const_iterator & end, vector<uint32_t> & cursor);
void findPaterns(vector<uint32_t> & data, map<uint32_t, pair<uint32_t, vector<uint32_t>>> & paterns);
void compressPatern(map<uint32_t, pair<uint32_t, vector<uint32_t>>> & paterns);
void renamePaterns(vector<uint32_t> & data, map<uint32_t, pair<uint32_t, vector<uint32_t>>> & paterns);
bool processPaterns(vector<uint32_t> & input, fstream & output, uint depth);

int main(int argc, const char * argv[])
{
	if(argc < 3)
	{
		cout << "Need an input and an output file" << endl;
		return -1;
	}
	
	if(argc < 2)
	{
		cout << "Need an input file" << endl;
		return -1;
	}
	
	vector<uint32_t> input;
	if(parseFile(argv[1], input))
		return -2;
	
	cout << "Read " << input.size() << " values." << endl;
	
	fstream stream;
	stream.open(argv[2], ios::out);
	
	int retValue = processPaterns(input, stream, 0) ? 0 : -3;
	
	stream.close();
	
	return retValue;
}

bool processPaterns(vector<uint32_t> & input, fstream & output, uint depth)
{
	//vector<pair<paternTag, pair<refCount, content>>>
	map<uint32_t, pair<uint32_t, vector<uint32_t>>> paterns;
	findPaterns(input, paterns);
	
	//Have we found any patern?
	if(paterns.size() == 0)
	{
		cout << "Couldn't find any patern!" << endl;
		return false;
	}
	
	cout << "Compressing the paterns we found!" << endl;
	
	compressPatern(paterns);
	renamePaterns(input, paterns);
	
	cout << "Compression over, writting everything down!" << endl;
	
	writeData(output, depth, paterns, input);
	
	return true;
}

bool parseFile(const char * fileName, vector<uint32_t> & vector)
{
	FILE * file = fopen(fileName, "r");
	if(file == NULL)
	{
		cout << "Couldn't open input file";
		return false;
	}

	uint32_t value = 0;
	while(fscanf(file, "0x%x\n", &value) != EOF)
	{
		if(value != 0)	//Ignore invalid values
		{
			if(value > PATERN_BOUNDARY)
				cout << "Value " << value << " > " << PATERN_BOUNDARY << ", this range is reserved, the value is ignored." << endl;
			else
				vector.push_back(value);
			
			value = 0;
		}
	}
	
	fclose(file);
	
	if(vector.size() == 0)
		cout << "Couldn't read the input file";

	
	return vector.size() == 0;
}

void writeData(fstream & stream, uint depth, map<uint32_t, pair<uint32_t, vector<uint32_t>>> & paterns, vector<uint32_t> & data)
{
	string padding(depth, '\t');
	
	stream << padding << "Paterns:";
	
	for(auto iter : paterns)
	{
		if(iter.second.first != 0)
		{
			stream << endl << padding << "ID " << "0x" << hex << iter.first << "(refCount: " << dec << iter.second.first << ")" << endl;
			
			auto & vector = iter.second.second;
			if(vector.size() < 50)
			{
				for(auto content : vector)
				{
					if(content == 0x42424242)	//access to the string
						stream << padding << "  Previous instruction accessed the string" << endl;
					else
						stream << padding << "0x" << hex << content << endl;
				}
			}
			else
			{
				stream << endl;
				processPaterns(vector, stream, depth + 1);
				stream << endl;
			}
		}
	}
	
	stream << endl << endl << padding << "Code:" << endl;
	
	for(auto iter : data)
	{
		if(iter == 0x42424242)	//access to the string
			stream << endl << padding << "	Previous instruction accessed the string";
		else
			stream << endl << padding << "0x" << hex << iter;
	}
}

bool fillCursorStartingAt(vector<uint32_t>::const_iterator & data, const vector<uint32_t>::const_iterator & end, vector<uint32_t> & cursor)
{
	cursor[0] = *data;

	//Not enough room
	if(++data == end)
		return false;
	
	cursor[1] = *data;
	return true;
}

void findPaterns(vector<uint32_t> & data, map<uint32_t, pair<uint32_t, vector<uint32_t>>> & paterns)
{
	vector<uint32_t> cursor(2);
	uint32_t paternTag = PATERN_BOUNDARY;
	for(vector<uint32_t>::const_iterator begin = data.begin(), mainIter = begin, end = data.end(); mainIter != end; )
	{
		//We just got a cursor we need to test in the array
		if(fillCursorStartingAt(mainIter, end, cursor))
		{
			//Try to find matches of our two slots cursor
			vector<size_t> indexesMatch;
			vector<uint32_t>::const_iterator searchIter = mainIter + 1;
			while(searchIter < end)
			{
				auto it = search(searchIter, end, cursor.begin(), cursor.end());
				
				if(it == end)
					break;
				
				indexesMatch.push_back(it - begin);
				searchIter = it + cursor.size();
			}
			
			//Found matches
			if(indexesMatch.size() > 0)
			{
				//Replace them in the array with the paternMatch tag
				for(vector<size_t>::reverse_iterator iter = indexesMatch.rbegin(), end = indexesMatch.rend(); iter != end; ++iter)
				{
					data.erase(begin + *iter + 1);
					data[*iter] = paternTag;
				}
				
				mainIter -= 1;	//mainIter was incremented in fillCursorStartingAt, we need to keep it at the begining of our cursor
				
				data.erase(mainIter + 1);
				data[mainIter - begin] = paternTag;
				
				end = data.end();
				
				//Insert the patern in the vector
				paterns.insert(make_pair(paternTag++, pair<uint32_t, vector<uint32_t>>(indexesMatch.size() + 1, cursor)));
			}
		}
	}
}

void compressPatern(map<uint32_t, pair<uint32_t, vector<uint32_t>>> & paterns)
{
	map<uint32_t, pair<uint32_t, vector<uint32_t>>>::reverse_iterator iter = paterns.rbegin();
	map<uint32_t, pair<uint32_t, vector<uint32_t>>>::reverse_iterator end = paterns.rend();
	vector<uint32_t> keyToRemove;
	
	for(iter++; iter != end; ++iter)
	{
		//refcount != 0
		if(iter->second.first != 0)
		{
			vector<uint32_t> & content = iter->second.second;	//Content of the patern
			
			for(size_t i = 0; i < content.size();)
			{
				if(content[i] >= PATERN_BOUNDARY)	//We got a patern in the patern!
				{
					uint32_t tokenToFind = content[i];
					//We look for the same patern in the vector
					auto search = paterns.find(tokenToFind);
					if(search != paterns.end())
					{
						//Paterns before this post processing are only composed of two items
						content[i] = search->second.second[1];
						content.insert(content.begin() + i, search->second.second[0]);
						
						//Update the reference counting
						search->second.first -= iter->second.first;
					}
				}
				else
					i += 1;
			}
		}
		else
			keyToRemove.push_back(iter->first);
	}
	
	for(auto key = keyToRemove.begin(); key != keyToRemove.end(); ++key)
		paterns.erase(*key);
}

void renamePaterns(vector<uint32_t> & data, map<uint32_t, pair<uint32_t, vector<uint32_t>>> & paterns)
{
	map<uint32_t, uint32_t> renameTable;
	uint32_t paternTag = PATERN_BOUNDARY;

	//Find paterns that need renaming
	for(auto iter = paterns.rbegin()++; iter != paterns.rend(); ++iter, ++paternTag)
	{
		if(iter->first != paternTag)
			renameTable.insert(make_pair(iter->first, paternTag));
	}

	//Update paterns name
	for(auto iter : renameTable)
	{
		auto data = paterns[iter.first];
		paterns.erase(iter.first);
		paterns.insert(make_pair(iter.second, data));
	}
	
	//Propagate the namechange to the content of the paterns
	for(auto iter = paterns.begin(); iter != paterns.end(); ++iter)
	{
		auto & vector = iter->second.second;
		for(auto iterContent = vector.begin(); iterContent != vector.end(); ++iterContent)
		{
			if(*iterContent >= PATERN_BOUNDARY)
			{
				auto needReplacement = renameTable.find(*iterContent);
				if(needReplacement != renameTable.end())
					*iterContent = needReplacement->second;
			}
		}
	}
	
	for(auto iterContent = data.begin(); iterContent != data.end(); ++iterContent)
	{
		if(*iterContent >= PATERN_BOUNDARY)
		{
			auto needReplacement = renameTable.find(*iterContent);
			if(needReplacement != renameTable.end())
				*iterContent = needReplacement->second;
		}
	}
}
