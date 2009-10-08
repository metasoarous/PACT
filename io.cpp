/* io.cpp
Copyright 2009 Trevor Bedford <bedfordt@umich.edu>
Member function definitions for IO class
*/

#include <iostream>
#include <fstream>
using std::ifstream;
using std::ofstream;
using std::cout;
using std::endl;
using std::ios;

#include <stdexcept>
using std::runtime_error;
using std::out_of_range;

#include <string>
using std::string;

#include <vector>
using std::vector;

#include "io.h"
#include "coaltree.h"
#include "series.h"

IO::IO() {

	inputFile = "in.trees";
	outputPrefix = "out";
	
	// ZEROING OUTPUT FILES ///////////
	// append from now on
	// only zero files that will be used later
	ofstream outStream;
	string outputFile;

	if (param.print_hp_tree) {
		outputFile = outputPrefix + ".rules";
		outStream.open( outputFile.c_str(),ios::out);
		outStream.close();
	}

	if (param.summary()) {
		outputFile = outputPrefix + ".stats";
		outStream.open( outputFile.c_str(),ios::out);
		outStream.close();
	}
	
	if (param.tips()) {
		outputFile = outputPrefix + ".tips";
		outStream.open( outputFile.c_str(),ios::out);
		outStream.close();
	}	

	if (param.skyline()) {
		outputFile = outputPrefix + ".skylines";
		outStream.open( outputFile.c_str(),ios::out);
		outStream.close();	
	}

	// PARAMETER INPUT ////////////////
	// automatically loaded by declaring Parameter object in header
	param.print();

	// TREE INPUT /////////////////////
	cout << "Reading trees from in.trees" << endl;
	
	ifstream inStream;
	inStream.open( inputFile.c_str(),ios::out);

	string line;
	int pos;
	if (inStream.is_open()) {
		while (! inStream.eof() ) {
			getline (inStream,line);
			
			if (line.size() > 0) {
				if (line[0] != '#') {
			
					// Catching log probabilities of trees
					string annoString;
					
					// migrate annotation
					annoString = "ln(L) = ";
					pos = line.find(annoString);
		
					if (pos >= 0) {
						string thisString;
						thisString = line.substr(pos+annoString.size());
						thisString.erase(thisString.find(' '));
						double ll = atof(thisString.c_str());
						problist.push_back(ll);
						line = "";								// ignore rest of line
					}
					
					// beast annotation
					annoString = "[&lnP=";
					pos = line.find(annoString);
		
					if (pos >= 0) {
						string thisString;
						thisString = line.substr(pos+annoString.size());
						thisString.erase(thisString.find(']'));
						double ll = atof(thisString.c_str());
						problist.push_back(ll);				
					}
					
					// find first occurance of '(' in line
					pos = line.find('(');
					if (pos >= 0) {
					
						string paren = line.substr(pos);
						
						CoalescentTree ct(paren);
						treelist.push_back(ct);
						cout << "tree " << treelist.size() << " read" << endl;
							
					}

				}
			}			
		}
		inStream.close();
	}
	else {
		throw runtime_error("input file not found");
	}
	
	cout << endl;
	
}

/* go through treelist and perform tree manipulation operations */
void IO::treeManip() {

	if (param.manip()) {

		cout << "Performing tree manipulation operations"  << endl;
		
		for (int i = 0; i < treelist.size(); i++) {
		
			// PUSH TIMES BACK
			if (param.push_times_back) {
				if ( (param.push_times_back_values).size() == 1 ) {
					double stop = (param.push_times_back_values)[0];
					treelist[i].pushTimesBack(stop);
				}
				if ( (param.push_times_back_values).size() == 2 ) {
					double start = (param.push_times_back_values)[0];
					double stop = (param.push_times_back_values)[1];
					treelist[i].pushTimesBack(start,stop);
				}			
			}
			
			// PRUNE TO TRUNK
			if (param.prune_to_trunk) {
				treelist[i].pruneToTrunk();
			}
			
			// PRUNE TO LABEL
			if (param.prune_to_label) {
				int label = (param.prune_to_label_values)[0];
				treelist[i].pruneToLabel(label);
			}
			
			// TRIM ENDS
			if (param.trim_ends) {
				double start = (param.trim_ends_values)[0];
				double stop = (param.trim_ends_values)[1];
				treelist[i].trimEnds(start,stop);
			}
			
			// SECTION TREE
			if (param.section_tree) {
				double start = (param.section_tree_values)[0];
				double window = (param.section_tree_values)[1];
				double step = (param.section_tree_values)[2];
				treelist[i].sectionTree(start,window,step);
			}
		
			// TIME SLICE
			if (param.time_slice) {
				double time = (param.time_slice_values)[0];
				treelist[i].timeSlice(time);
			}
		
		}

	}

}

/* go through problist and treelist and print highest posterior probability tree */
void IO::printHPTree() {

	if (param.print_hp_tree) {

		string outputFile = outputPrefix + ".rules";
		cout << "Printing tree with highest posterior probability to " << outputFile << endl;
	
		// output tree with highest likelihood
		// if likelihoods don't exist, output final tree
		
		int index;
		if (problist.size() == treelist.size()) {
			
			double ll = problist[0];
			index = 0;
			for (int i = 1; i < problist.size(); i++) {
				if (problist[i] > ll) {
					ll = problist[i];
					index = i;
				}
			}
			
		}
		else {
			index = treelist.size() - 1;
		}
		
		treelist[index].printRuleList(outputFile);
	
	}

}

/* go through treelist and summarize coalescent statistics */
void IO::printStatistics() {

	if (param.summary()) {

		/* initializing output stream */
		string outputFile = outputPrefix + ".stats";
		ofstream outStream;
		outStream.open( outputFile.c_str(),ios::app);
		
		outStream << "statistic\tlower\tmean\tupper" << endl; 
		
		int maxLabel = treelist[0].getMaxLabel();

		// LABEL PROPORTIONS //////////////
		if (param.summary_proportions) {
			cout << "Printing trunk proportion summary to " << outputFile << endl;
			for (int label = 1; label <= maxLabel; label++) {
				Series s;
				for (int i = 0; i < treelist.size(); i++) {
					CoalescentTree ct = treelist[i];
			//		ct.pruneToTrunk();
					double n = ct.getLabelPro(label);
					s.insert(n);
				}
				outStream << "pro_" << label << "\t";
				outStream << s.quantile(0.025) << "\t" << s.mean() << "\t" << s.quantile(0.975) << endl;		
			}
		}
	
		// COALESCENCE /////////////////////
		if (param.summary_coal_rates) {
			cout << "Printing coalescent summary to " << outputFile << endl;	
			for (int label = 1; label <= maxLabel; label++) {
				Series s;
				for (int i = 0; i < treelist.size(); i++) {
					double n = treelist[i].getCoalRate(label);
					s.insert(n);
				}
				outStream << "coal_" << label << "\t";
				outStream << s.quantile(0.025) << "\t" << s.mean() << "\t" << s.quantile(0.975) << endl;
			}
		}
		
		// MIGRATION ///////////////////////
		if (param.summary_mig_rates) {		
			cout << "Printing migration summary to " << outputFile << endl;
			for (int from = 1; from <= maxLabel; from++) {
				for (int to = 1; to <= maxLabel; to++) {	
					if (from != to) {
		
						Series s;
						for (int i = 0; i < treelist.size(); i++) {
							double n = treelist[i].getMigRate(from,to);
							s.insert(n);
						}
						outStream << "mig_" << from << "_" << to << "\t";
						outStream << s.quantile(0.025) << "\t" << s.mean() << "\t" << s.quantile(0.975) << endl;
		
					}
				}	
			}
		}
				

		outStream.close();
	
	}

}

/* go through treelist and summarize tip statistics */
void IO::printTips() {

	if (param.tips()) {

		/* initializing output stream */
		string outputFile = outputPrefix + ".tips";
		ofstream outStream;
		outStream.open( outputFile.c_str(),ios::app);
		
		outStream << "statistic\tname\tlabel\ttime\tlower\tmean\tupper" << endl; 
		
		/* get vector of tip names */
		vector<string> tipNames = treelist[0].getTipNames();
		
		// TIME TO TRUNK //////////////
		if (param.tips_time_to_trunk) {
			cout << "Printing time to trunk for tips to " << outputFile << endl;
			for (int n = 0; n < tipNames.size(); n++) {
				string tip = tipNames[n];
				Series s;
				for (int i = 0; i < treelist.size(); i++) {
					double n = treelist[i].timeToTrunk(tip);
					s.insert(n);
				}
				outStream << "time_to_trunk" << "\t";
				outStream << tip << "\t";
				outStream << treelist[0].getLabel(tip) << "\t";
				outStream << treelist[0].getTime(tip) << "\t";				
				outStream << s.quantile(0.025) << "\t" << s.mean() << "\t" << s.quantile(0.975) << endl;		
			}
		}
	
		outStream.close();
	
	}

}

/* go through treelist and calculate skyline statistics */
void IO::printSkylines() {

	if (param.skyline()) {

		/* initializing output stream */
		string outputFile = outputPrefix + ".skylines";
		ofstream outStream;
		outStream.open( outputFile.c_str(),ios::app);
		
		outStream << "statistic\ttime\tlower\tmean\tupper" << endl; 
		
		int maxLabel = treelist[0].getMaxLabel();
		double start = param.skyline_values[0];
		double stop = param.skyline_values[1];
		double step = param.skyline_values[2];
	
		// COALESCENCE /////////////////////
		if (param.skyline_coal_rates) {
			cout << "Printing coalescent skyline to " << outputFile << endl;
			
			for (int label = 1; label <= maxLabel; label++) {
				for (double t = start; t + step <= stop; t += step) {
			
					Series s;
					for (int i = 0; i < treelist.size(); i++) {
						CoalescentTree ct = treelist[i];
						ct.trimEnds(t,t+step);
						double n = ct.getCoalRate(label);
						s.insert(n);
					}
					outStream << "coal_" << label << "\t";
					outStream << t + step / (double) 2 << "\t";
					outStream << s.quantile(0.025) << "\t" << s.mean() << "\t" << s.quantile(0.975) << endl;
					
				}
			}
		}
		
		// LABEL PROPORTIONS /////////////////////
		if (param.skyline_proportions) {
			cout << "Printing trunk proportions skyline to " << outputFile << endl;
			
			for (int label = 1; label <= maxLabel; label++) {
				for (double t = start; t + step <= stop; t += step) {
			
					Series s;
					for (int i = 0; i < treelist.size(); i++) {
						CoalescentTree ct = treelist[i];
						ct.trimEnds(t,t+step);
					//	ct.pruneToTrunk();
						double n = ct.getLabelPro(label);
						s.insert(n);
					}
					outStream << "pro_" << label << "\t";
					outStream << t + step / (double) 2 << "\t";
					outStream << s.quantile(0.025) << "\t" << s.mean() << "\t" << s.quantile(0.975) << endl;
					
				}
			}
		}		
		
		outStream.close();
	
	}

}