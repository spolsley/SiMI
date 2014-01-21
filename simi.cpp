/* ===========================================================
 * SiMI
 * Description: (Si)mple (MI)PS pipeline simulator
 * Author: Seth Polsley
 * Contact: spolsley@ku.edu
 * Modified: 12/12/13
 * ===========================================================
 */
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <string>
#include <boost/regex.hpp>
using namespace std;

//Struct Definitions
//Instruction holds each instruction
typedef struct {
	string op,label;
	int rs,rt,rd,offset;
} Instruction;

//Memory holds both data and instructions
typedef struct {
	int data[128];
	Instruction inst[128]; 
	void initMemory() {
		Instruction nop;
		nop.label = "";
		nop.op = "nop";
		nop.rs = -1;
		nop.rt = -1;
		nop.rd = -1;
		nop.offset = 0;
		for(int i=0; i<128; i++)
		{
			data[i] = 0;
			inst[i] = nop;
		}
	}
} Memory;

//Processor holds the registers and pipeline registers
typedef struct {
	int reg[32];
	Instruction pipeReg[8];
	void initProcessor() {
		Instruction nop;
		nop.label = "";
		nop.op = "nop";
		nop.rs = -1;
		nop.rt = -1;
		nop.rd = -1;
		nop.offset = 0;
		for(int i=0; i<32; i++)
			reg[i] = 0;
		for(int i=0; i<8; i++)
			pipeReg[i] = nop;
	};
	bool empty() {
		for(int i=0; i<8; i++)
		{
			if (pipeReg[i].op != "nop")
				return false;
		}
		return true;
	}
} Processor;

void parseInput(ifstream&, Processor&, Memory&);

//Main program
int main(int argc, char* argv[])
{
	string inFile = "";
	string outFile = "";

	Memory mem;
	Processor cpu;
	Instruction nop;
	bool reserve[32];
	bool stall;
	bool nextstall;
	bool branch;
	int i;
	int nexti;
	int c;
	int issue;
	char key;

	//Allow command line arguments
	if (argc > 1)
	{
		inFile = argv[1];
		outFile = argv[2];
	}
	do
	{
		//Main program loop
		if ((inFile == "")||(outFile == ""))
		{
			cout << "Please provide an input file: \n";
			cin >> inFile;
			cout << "Please provide an output file: \n";
			cin >> outFile;
			cout << "Simulation started.\n";
		}
		ifstream fin(inFile.c_str());
		ofstream fout(outFile.c_str());

		//System configuration
		mem.initMemory();
		cpu.initProcessor(); 

		parseInput(fin,cpu,mem);
		nop.label = "";
		nop.op = "nop";
		nop.rs = -1;
		nop.rt = -1;
		nop.rd = -1;
		nop.offset = 0;
		for (int i = 0; i < 32; i++)
			reserve[i] = false;
		stall = false;
		nextstall = false;
		branch = false;
		i = 0;
		nexti = 0;
		c = 1;
		issue = 1;

		//Main pipeline loop
		do
		{
			fout << "c#" << c;
			//WB
			if (!(cpu.pipeReg[7].op == "nop"))
			{
				fout << " " << cpu.pipeReg[7].label << "-WB";
			}
			//MEM3
			if (!(cpu.pipeReg[6].op == "nop"))
			{
				fout << " " << cpu.pipeReg[6].label << "-MEM3";
				cpu.pipeReg[7] = cpu.pipeReg[6];
			}
			else cpu.pipeReg[7] = nop;
			//MEM2
			if (!(cpu.pipeReg[5].op == "nop"))
			{
				if (cpu.pipeReg[5].op == "LD")
				{
					cpu.reg[cpu.pipeReg[5].rd] = mem.data[(cpu.pipeReg[5].rs+cpu.pipeReg[5].offset)/8];
					//Clear waiting register
					reserve[cpu.pipeReg[5].rd] = false;
				}
				else if (cpu.pipeReg[5].op == "SD")
				{
					mem.data[(cpu.pipeReg[5].rs+cpu.pipeReg[5].offset)/8] = cpu.pipeReg[5].rt;
				}
				fout << " " << cpu.pipeReg[5].label << "-MEM2";
				cpu.pipeReg[6] = cpu.pipeReg[5];
			}
			else cpu.pipeReg[6] = nop;
			//MEM1
			if (!(cpu.pipeReg[4].op == "nop"))
			{
				fout << " " << cpu.pipeReg[4].label << "-MEM1";
				cpu.pipeReg[5] = cpu.pipeReg[4];
			}
			else cpu.pipeReg[5] = nop;
			//EX
			if (!(cpu.pipeReg[3].op == "nop"))
			{
				if (cpu.pipeReg[3].op == "BNEZ")
				{
					if (cpu.pipeReg[3].rs != 0)
					{
						branch = true;
						//Update PC if branch taken
						nexti = cpu.pipeReg[3].offset;
					}
				}
				//Perform ALU calculations
				else if ((cpu.pipeReg[3].op == "DADD")||(cpu.pipeReg[3].op == "DADDI"))
				{
					if (cpu.pipeReg[3].rt > 0)
						cpu.reg[cpu.pipeReg[3].rd] = cpu.pipeReg[3].rs + cpu.pipeReg[3].rt;
					else
						cpu.reg[cpu.pipeReg[3].rd] = cpu.pipeReg[3].rs + cpu.pipeReg[3].offset;
				}
				else if ((cpu.pipeReg[3].op == "SUB")||(cpu.pipeReg[3].op == "SUBI"))
				{
					if (cpu.pipeReg[3].rt > 0)
						cpu.reg[cpu.pipeReg[3].rd] = cpu.pipeReg[3].rs - cpu.pipeReg[3].rt;
					else
						cpu.reg[cpu.pipeReg[3].rd] = cpu.pipeReg[3].rs - cpu.pipeReg[3].offset;
				}
				fout << " " << cpu.pipeReg[3].label << "-EX";
				cpu.pipeReg[4] = cpu.pipeReg[3];
			}
			else cpu.pipeReg[4] = nop;
			//ID
			if (!(cpu.pipeReg[2].op == "nop"))
			{
				if (cpu.pipeReg[2].op == "LD")
					//Set waiting register
					reserve[cpu.pipeReg[2].rd] = true;
				//Hazard detection
				if ((reserve[cpu.pipeReg[2].rs]||reserve[cpu.pipeReg[2].rt])&&(!stall))
				{
					fout << " " << cpu.pipeReg[2].label << "-ID";
					cpu.pipeReg[3] = nop;
					nextstall = true;
				}
				else if ((reserve[cpu.pipeReg[2].rs]||reserve[cpu.pipeReg[2].rt])&&(stall))
				{
					fout << " " << cpu.pipeReg[2].label << "-stall";
					cpu.pipeReg[3] = nop;
				}
				else if (!(reserve[cpu.pipeReg[2].rs]||reserve[cpu.pipeReg[2].rt])&&(stall))
				{
					fout << " " << cpu.pipeReg[2].label << "-stall";
					cpu.pipeReg[2].rs = cpu.reg[cpu.pipeReg[2].rs];
					cpu.pipeReg[2].rt = cpu.reg[cpu.pipeReg[2].rt];
					cpu.pipeReg[3] = cpu.pipeReg[2];
					nextstall = false;
				}
				else
				{
					fout << " " << cpu.pipeReg[2].label << "-ID";
					cpu.pipeReg[2].rs = cpu.reg[cpu.pipeReg[2].rs];
					cpu.pipeReg[2].rt = cpu.reg[cpu.pipeReg[2].rt];
					cpu.pipeReg[3] = cpu.pipeReg[2];
				}
			}
			else cpu.pipeReg[3] = nop;
			//IF2
			if (!(cpu.pipeReg[1].op == "nop"))
			{
				if (nextstall&&(!stall))
					fout << " " << cpu.pipeReg[1].label << "-IF2";
				else if (nextstall&&stall)
					fout << " " << cpu.pipeReg[1].label << "-stall";
				else if ((!nextstall)&&stall)
				{
					fout << " " << cpu.pipeReg[1].label << "-stall";
					cpu.pipeReg[2] = cpu.pipeReg[1];
				}
				else
				{
					fout << " " << cpu.pipeReg[1].label << "-IF2";
					cpu.pipeReg[2] = cpu.pipeReg[1];
				}
			}
			else cpu.pipeReg[2] = nop;
			//IF1
			if (!(mem.inst[i].op == "nop"))
			{
				cpu.pipeReg[0] = mem.inst[i];
				cpu.pipeReg[0].label = "I" + to_string(issue);
				if (nextstall&&(!stall))
					fout << " " << cpu.pipeReg[0].label << "-IF1";
				else if (nextstall&&stall)
					fout << " " << cpu.pipeReg[0].label << "-stall";
				else if ((!nextstall)&&stall)
				{
					fout << " " << cpu.pipeReg[0].label << "-stall";
					cpu.pipeReg[1] = cpu.pipeReg[0];
					i++;
					issue++;
				}
				else
				{
					fout << " " << cpu.pipeReg[0].label << "-IF1";
					cpu.pipeReg[1] = cpu.pipeReg[0];
					i++;
					issue++;
				}
			}
			else
			{
				if (((!nextstall)&&stall)||((!nextstall)&&(!stall)))
					cpu.pipeReg[1] = nop;
				cpu.pipeReg[0] = nop;
			}

			//Set stall state for next cycle
			stall = nextstall;
			//Update instructions if branching
			if (branch)
			{
				cpu.pipeReg[3] = nop;
				cpu.pipeReg[2] = nop;
				cpu.pipeReg[1] = nop;
				i = nexti;
				branch = false;
			}
			fout << endl;
			//Update cycle count and break if all instructions complete
			c++;
			if (cpu.empty()) break;
		} while(i < 128);
		//Print out non-zero register and memory values
		fout << "REGISTERS\n";
		for (int i = 0; i < 32; i++)
		{
			if (cpu.reg[i] != 0)
				fout << "R" << i << " " << cpu.reg[i] << endl;
		}
		fout << "MEMORY\n";
		for (int i = 0; i < 128; i++)
		{
			if (mem.data[i] != 0)
				fout << i*8 << " " << mem.data[i] << endl;
		}
		fout << endl;
		fin.close();
		fout.close();
		cout << "Simulation complete.\n";
		inFile = "";
		outFile = "";
		cout << "Enter q to exit. Enter any other key to run the simulation again.\n";
		cin >> key;
	} while(key != 'q');
	return 0;
}				

//Helper function
//parseInput accepts the file and sets the processor and memory state
void parseInput(ifstream& fin, Processor& cpu, Memory& mem)
{
	string line;
	string dataA,dataB;
	int intA,intB;

	//Set registers
	getline(fin,line);
	while(getline(fin,line))
	{
		//Ignore empty lines
		if((line=="\n")||(line==" \r")||(line=="\r")||(line=="\t\r")) continue;
		if((signed int)line.find("MEMORY") >= 0) break;
		istringstream input(line);
		input >> dataA >> intA;
		dataA = dataA.substr(1,dataA.length()-1);
		int t = atoi(dataA.c_str());
		cpu.reg[t] = intA;
	}

	//Set memory values
	while(getline(fin,line))
	{
		//Ignore empty lines
		if((line=="\n")||(line==" \r")||(line=="\r")||(line=="\t\r")) continue;
		if((signed int)line.find("CODE") >= 0) break;
		istringstream input(line);
		input >> intA >> intB;
		mem.data[intA/8] = intB;
	}

	//Preprocess the instructions
	//This single regex separates each instruction line into individual parts
	boost::regex ex("(?:(.*)(?::))?(?:(?:[\\s\\t]*)?((?:[^\\s\\,]*)))(?:[\\s\\t]*)(R\\d?\\d)(?:[\\,\\s\\t]*)([^\\,\\s\\t\\(]*)(?:(?:[\\(])(?:[\\s]*)?([^\\)\\s]*)(?:[\\s]*)?(?:[\\)]))?(?:(?:[\\,\\s\\t]*)([^\\s]*)(?:[\\n\\s\\t\\f\\r]*)?)?");
	boost::smatch sm;
	string labels[128];
	for (int i = 0; i < 128; i++)
		labels[i] = "";
	int i = 0;
	while(getline(fin,line))
	{
		//Ignore empty lines
		if((line=="\n")||(line==" \r")||(line=="\r")||(line=="\t\r")) continue;
		regex_match(line,sm,ex);
		if (sm.size() > 0)
		{
			Instruction newInst;
			newInst.label = sm[1];
			newInst.op = sm[2];
			newInst.rs = -1;
			newInst.rt = -1;
			newInst.rd = -1;
			newInst.offset = 0;
			string a = sm[3];
			string b = sm[4];
			string c = sm[5];
			string d = sm[6];
			if (newInst.op == "LD")
			{
				newInst.rd = atoi(a.substr(1,a.length()-1).c_str());
				newInst.offset = atoi(b.c_str());
				newInst.rs = atoi(c.substr(1,c.length()-1).c_str());
			}
			else if (newInst.op == "SD")
			{
				newInst.rt = atoi(a.substr(1,a.length()-1).c_str());
				newInst.offset = atoi(b.c_str());
				newInst.rs = atoi(c.substr(1,c.length()-1).c_str());
			}
			else if (newInst.op == "BNEZ")
			{
				newInst.rs = atoi(a.substr(1,a.length()-1).c_str());
				labels[i] = b;
			}
			else
			{
				newInst.rd = atoi(a.substr(1,a.length()-1).c_str());
				newInst.rs = atoi(b.substr(1,b.length()-1).c_str());
				if ((signed int)d.find("#") >= 0)
					newInst.offset = atoi(d.substr(1,d.length()-1).c_str());
				else
					newInst.rt = atoi(d.substr(1,d.length()-1).c_str());
			}
			mem.inst[i] = newInst;
		}
		i++;
	}
	//Assign branch offsets to point to the targets in memory
	for (int i = 0; i < 128; i++)
	{
		if (labels[i] != "")
		{
			for (int j = 0; j < 128; j++)
			{
				if (mem.inst[j].label == labels[i])
				{
					mem.inst[i].offset = j;
					break;
				}
			}
		}
	}
}
