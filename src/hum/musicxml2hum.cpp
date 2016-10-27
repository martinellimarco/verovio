//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Sat Aug  6 10:53:40 CEST 2016
// Last Modified: Sun Sep 18 14:16:18 PDT 2016
// Filename:      musicxml2hum.cpp
// URL:           https://github.com/craigsapp/hum2ly/blob/master/src/musicxml2hum.cpp
// Syntax:        C++11
// vim:           ts=3 noexpandtab
//
// Description:   Convert a MusicXML file into a Humdrum file.
//

#include "musicxml2hum.h"
#include "HumGrid.h"

using namespace std;
using namespace pugi;

namespace hum {


//////////////////////////////
//
// musicxml2hum_interface::musicxml2hum_interface --
//

musicxml2hum_interface::musicxml2hum_interface(void){
	// Options& options = m_options;
	// options.define("k|kern=b","display corresponding **kern data");
}



//////////////////////////////
//
// musicxml2hum_interface::convert -- Convert a MusicXML file into
//     Humdrum content.
//

bool musicxml2hum_interface::convertFile(ostream& out,
		const char* filename) {
	xml_document doc;
	auto result = doc.load_file(filename);
	if (!result) {
		cerr << "\nXML file [" << filename << "] has syntax errors\n";
		cerr << "Error description:\t" << result.description() << "\n";
		cerr << "Error offset:\t" << result.offset << "\n\n";
		exit(1);
	}

	return convert(out, doc);
}


bool musicxml2hum_interface::convert(ostream& out, istream& input) {
	string s(istreambuf_iterator<char>(input), {});
	return convert(out, s.c_str());
}


bool musicxml2hum_interface::convert(ostream& out, const char* input) {
	xml_document doc;
	auto result = doc.load(input);
	if (!result) {
		cout << "\nXML content has syntax errors\n";
		cout << "Error description:\t" << result.description() << "\n";
		cout << "Error offset:\t" << result.offset << "\n\n";
		exit(1);
	}

	return convert(out, doc);
}


bool musicxml2hum_interface::convert(ostream& out, xml_document& doc) {
	bool status = true; // for keeping track of problems in conversion process.

	vector<string> partids;            // list of part IDs
	map<string, xml_node> partinfo;    // mapping if IDs to score-part elements
	map<string, xml_node> partcontent; // mapping of IDs to part elements

	getPartInfo(partinfo, partids, doc);
	getPartContent(partcontent, partids, doc);
	vector<MxmlPart> partdata;
	partdata.resize(partids.size());
	fillPartData(partdata, partids, partinfo, partcontent);

	// for debugging:
	// printPartInfo(partids, partinfo, partcontent, partdata);

	HumGrid outdata;
	status &= stitchParts(outdata, partids, partinfo, partcontent, partdata);

	HumdrumFile outfile;
	outdata.transferTokens(outfile);
	for (int i=0; i<outfile.getLineCount(); i++) {
		outfile[i].createLineFromTokens();
	}
	out << outfile;

	return status;
}



//////////////////////////////
//
// musicxml2hum_interface::setOptions --
//

void musicxml2hum_interface::setOptions(int argc, char** argv) {
	m_options.process(argc, argv);
}


void musicxml2hum_interface::setOptions(const vector<string>& argvlist) {
	int tempargc = (int)argvlist.size();
	char* tempargv[tempargc+1];
	tempargv[tempargc] = NULL;
	
	int i;
	for (i=0; i<tempargc; i++) {
		tempargv[i] = new char[argvlist[i].size() + 1];
		strcpy(tempargv[i], argvlist[i].c_str());
	}

	setOptions(tempargc, tempargv);

	for (i=0; i<tempargc; i++) {
		if (tempargv[i] != NULL) {
			delete [] tempargv[i];
		}
	}
}



//////////////////////////////
//
// musicxml2hum_interface::getOptionDefinitions -- Used to avoid
//     duplicating the definitions in the test main() function.
//

Options musicxml2hum_interface::getOptionDefinitions(void) {
	return m_options;
}


///////////////////////////////////////////////////////////////////////////


//////////////////////////////
//
// musicxml2hum_interface::fillPartData --
//

bool musicxml2hum_interface::fillPartData(vector<MxmlPart>& partdata,
		const vector<string>& partids, map<string, xml_node>& partinfo,
		map<string, xml_node>& partcontent) {

	bool output = true;
	for (int i=0; i<(int)partinfo.size(); i++) {
		partdata[i].setPartNumber(i+1);
		output &= fillPartData(partdata[i], partids[i], partinfo[partids[i]],
				partcontent[partids[i]]);
	}
	return output;
}


bool musicxml2hum_interface::fillPartData(MxmlPart& partdata,
		const string& id, xml_node partdeclaration, xml_node partcontent) {
	auto measures = partcontent.select_nodes("./measure");
	for (int i=0; i<(int)measures.size(); i++) {
		partdata.addMeasure(measures[i].node());
	}
	return true;
}



//////////////////////////////
//
// musicxml2hum_interface::printPartInfo -- Debug information.
//

void musicxml2hum_interface::printPartInfo(vector<string>& partids,
		map<string, xml_node>& partinfo, map<string, xml_node>& partcontent,
		vector<MxmlPart>& partdata) {
	cout << "\nPart information in the file:" << endl;
	int maxmeasure = 0;
	for (int i=0; i<partids.size(); i++) {
		cout << "\tPART " << i+1 << " id = " << partids[i] << endl;
		cout << "\tMAXSTAFF " << partdata[i].getStaffCount() << endl;
		cout << "\t\tpart name:\t"
		     << getChildElementText(partinfo[partids[i]], "part-name") << endl;
		cout << "\t\tpart abbr:\t"
		     << getChildElementText(partinfo[partids[i]], "part-abbreviation")
		     << endl;
		auto node = partcontent[partids[i]];
		auto measures = node.select_nodes("./measure");
		cout << "\t\tMeasure count:\t" << measures.size() << endl;
		if (maxmeasure < (int)measures.size()) {
			maxmeasure = (int)measures.size();
		}
		cout << "\t\tTotal duration:\t" << partdata[i].getDuration() << endl;
	}

	MxmlMeasure* measure;
	for (int i=0; i<maxmeasure; i++) {
		cout << "m" << i+1 << "\t";
		for (int j=0; j<(int)partdata.size(); j++) {
			measure = partdata[j].getMeasure(i);
			if (measure) {
				cout << measure->getDuration();
			}
			if (j < (int)partdata.size() - 1) {
				cout << "\t";
			}
		}
		cout << endl;
	}
}



//////////////////////////////
//
// stitchParts -- Merge individual parts into a single score sequence.
//

bool musicxml2hum_interface::stitchParts(HumGrid& outdata,
		vector<string>& partids, map<string, xml_node>& partinfo,
		map<string, xml_node>& partcontent, vector<MxmlPart>& partdata) {

	if (partdata.size() == 0) {
		return false;
	}

//	insertExclusiveInterpretationLine(outfile, partdata);

	int i;
	int measurecount = partdata[0].getMeasureCount();
	for (i=1; i<(int)partdata.size(); i++) {
		if (measurecount != partdata[i].getMeasureCount()) {
			cerr << "ERROR: cannot handle parts with different measure\n";
			cerr << "counts yet. Compare " << measurecount << " to ";
			cerr << partdata[i].getMeasureCount() << endl;
			exit(1);
		}
	}

	vector<int> partstaves(partdata.size(), 0);
	for (i=0; i<partstaves.size(); i++) {
		partstaves[i] = partdata[i].getStaffCount();
	}

	// vector<HumdrumLine*> measures;

	bool status = true;
	int m;
	for (m=0; m<partdata[0].getMeasureCount(); m++) {
		status &= insertMeasure(outdata, m, partdata, partstaves);
		// a hack for now:
		// insertSingleMeasure(outfile);
		// measures.push_back(&outfile[outfile.getLineCount()-1]);
	}

// Do this later, maybe in another class:
//	insertAllToken(outfile, partdata, "*-");
//	cleanupMeasures(outfile, measures);

	return status;
}



//////////////////////////////
//
// musicxml2hum_interface::cleanupMeasures --
//     Also add barlines here (keeping track of the 
//     duration of each measure).
//

void musicxml2hum_interface::cleanupMeasures(HumdrumFile& outfile,
		vector<HumdrumLine*> measures) {

   HumdrumToken* token;
	for (int i=0; i<outfile.getLineCount(); i++) {
		if (!outfile[i].isBarline()) {
			continue;
		}
		if (!outfile[i+1].isInterpretation()) {
			int fieldcount = outfile[i+1].getFieldCount();
			for (int j=1; j<fieldcount; j++) {
				token = new HumdrumToken("=");
				outfile[i].appendToken(token);
			}
		}
	}
}



//////////////////////////////
//
// musicxml2hum_interface::insertExclusiveInterpretationLine --
//

void musicxml2hum_interface::insertExclusiveInterpretationLine(
		HumdrumFile& outfile, vector<MxmlPart>& partdata) {

	HumdrumLine* line = new HumdrumLine;
	HumdrumToken* token;

	int i, j;
	for (i=0; i<(int)partdata.size(); i++) {
		for (j=0; j<(int)partdata[i].getStaffCount(); j++) {
			token = new HumdrumToken("**kern");
			line->appendToken(token);
		}
		for (j=0; j<(int)partdata[i].getVerseCount(); j++) {
			token = new HumdrumToken("**kern");
			line->appendToken(token);
		}
	}
	outfile.appendLine(line);
}



//////////////////////////////
//
// musicxml2hum_interface::insertSingleMeasure --
//

void musicxml2hum_interface::insertSingleMeasure(HumdrumFile& outfile) {
	HumdrumLine* line = new HumdrumLine;
	HumdrumToken* token;
	token = new HumdrumToken("=");
	line->appendToken(token);
	line->createLineFromTokens();
	outfile.appendLine(line);
}



//////////////////////////////
//
// musicxml2hum_interface::insertAllToken --
//

void musicxml2hum_interface::insertAllToken(HumdrumFile& outfile,
		vector<MxmlPart>& partdata, const string& common) {

	HumdrumLine* line = new HumdrumLine;
	HumdrumToken* token;

	int i, j;
	for (i=0; i<(int)partdata.size(); i++) {
		for (j=0; j<(int)partdata[i].getStaffCount(); j++) {
			token = new HumdrumToken(common);
			line->appendToken(token);
		}
		for (j=0; j<(int)partdata[i].getVerseCount(); j++) {
			token = new HumdrumToken(common);
			line->appendToken(token);
		}
	}
	outfile.appendLine(line);
}



//////////////////////////////
//
// musicxml2hum_interface::insertMeasure --
//

bool musicxml2hum_interface::insertMeasure(HumGrid& outdata, int mnum,
		vector<MxmlPart>& partdata, vector<int> partstaves) {

	GridMeasure* gm = new GridMeasure;
	outdata.push_back(gm);

	vector<MxmlMeasure*> measuredata;
	vector<vector<SimultaneousEvents>* > sevents;
	int i;
	for (i=0; i<(int)partdata.size(); i++) {
		measuredata.push_back(partdata[i].getMeasure(mnum));
		sevents.push_back(measuredata.back()->getSortedEvents());
	}

	vector<HumNum> curtime(partdata.size());
	vector<int> curindex(partdata.size(), 0); // assuming data in a measure...
	HumNum nexttime = -1;
	for (i=0; i<(int)curtime.size(); i++) {
		curtime[i] = (*sevents[i])[curindex[i]].starttime;
		if (nexttime < 0) {
			nexttime = curtime[i];
		} else if (curtime[i] < nexttime) {
			nexttime = curtime[i];
		}
	}


	bool allend = false;
	vector<SimultaneousEvents*> nowevents;
	vector<int> nowparts;
	bool status = true;
	while (!allend) {
		nowevents.resize(0);
		nowparts.resize(0);
		allend = true;
		HumNum processtime = nexttime;
		nexttime = -1;
		for (i = (int)partdata.size()-1; i >= 0; i--) {
			if (curindex[i] >= (int)(*sevents[i]).size()) {
				continue;
			}
			if ((*sevents[i])[curindex[i]].starttime == processtime) {
				nowevents.push_back(&(*sevents[i])[curindex[i]]);
				nowparts.push_back(i);
				curindex[i]++;
			}
			if (curindex[i] < (int)(*sevents[i]).size()) {
				allend = false;
				if ((nexttime < 0) ||
						((*sevents[i])[curindex[i]].starttime < nexttime)) {
					nexttime = (*sevents[i])[curindex[i]].starttime;
				}
			}
		}
		status &= convertNowEvents(*outdata.back(),
		                         nowevents,
		                         nowparts,
		                         processtime,
		                         partdata,
		                         partstaves);
	}

	return status;
}



//////////////////////////////
//
// musicxml2hum_interface::convertNowEvents --
//

bool musicxml2hum_interface::convertNowEvents(
		GridMeasure& outdata,
		vector<SimultaneousEvents*>& nowevents,
		vector<int>& nowparts,
		HumNum nowtime,
		vector<MxmlPart>& partdata,
		vector<int>& partstaves) {

	if (nowevents.size() == 0) {
		// cout << "NOW EVENTS ARE EMPTY" << endl;
		return true;
	}

	appendZeroEvents(outdata, nowevents, nowtime, partdata);

	if (nowevents[0]->nonzerodur.size() == 0) {
		// no duration events (should be a terminal barline)
		// ignore and deal with in calling function.
		return true;
	}

	appendNonZeroEvents(outdata, nowevents, nowtime, partdata);

	return true;
}



/////////////////////////////
//
// musicxml2hum_interface::appendNonZeroEvents --
//

void musicxml2hum_interface::appendNonZeroEvents(
		GridMeasure& outdata,
		vector<SimultaneousEvents*>& nowevents,
		HumNum nowtime,
		vector<MxmlPart>& partdata) {

	GridSlice* slice = new GridSlice(nowtime, SliceType::Notes);
	outdata.push_back(slice);
	slice->initializePartStaves(partdata);

	for (int i=0; i<(int)nowevents.size(); i++) {
		vector<MxmlEvent*>& events = nowevents[i]->nonzerodur;
		for (int j=0; j<(int)events.size(); j++) {
			addEvent(*slice, events[j]);
		}
	}
}



//////////////////////////////
//
// musicxml2hum_interface::addEvent --
//

void musicxml2hum_interface::addEvent(GridSlice& slice,
		MxmlEvent* event) {

	int partindex;  // which part the event occurs in
	int staffindex; // which staff the event occurs in (need to fix)
	int voiceindex; // which voice the event occurs in (use for staff)

	partindex  = event->getPartIndex();
	staffindex = event->getStaffIndex();
	voiceindex = event->getVoiceIndex();

	string recip   = event->getRecip();
	string pitch   = event->getKernPitch();
	string prefix  = event->getPrefixNoteInfo();
	string postfix = event->getPostfixNoteInfo();
	stringstream ss;
	ss << prefix << recip << pitch << postfix;

	HTp token = new HumdrumToken(ss.str());
	slice.at(partindex)->at(staffindex)->setTokenLayer(voiceindex, token,
		event->getDuration());
}



/////////////////////////////
//
// musicxml2hum_interface::appendZeroEvents --
//

void musicxml2hum_interface::appendZeroEvents(
		GridMeasure& outdata,
		vector<SimultaneousEvents*>& nowevents,
		HumNum nowtime,
		vector<MxmlPart>& partdata) {

	bool hasclef    = false;
	bool haskeysig  = false;
	bool hastimesig = false;

	vector<xml_node> clefs(partdata.size(), xml_node(NULL));
	vector<xml_node> keysigs(partdata.size(), xml_node(NULL));
	vector<xml_node> timesigs(partdata.size(), xml_node(NULL));
	int pindex = 0;
	xml_node child;

	for (int i=0; i<nowevents.size(); i++) {
		for (int j=0; j<nowevents[i]->zerodur.size(); j++) {
			xml_node element = nowevents[i]->zerodur[j]->getNode();
			if (nodeType(element, "attributes")) {
				child = element.first_child();
				while (child) {
					pindex = nowevents[i]->zerodur[j]->getPartIndex();

					if (nodeType(child, "clef") && !clefs[pindex]) {
						clefs[pindex] = child;
						hasclef = true;
					}

					if (nodeType(child, "key") && !keysigs[pindex]) {
						keysigs[pindex] = child;
						haskeysig = true;
					}

					if (nodeType(child, "time") && !timesigs[pindex]) {
						timesigs[pindex] = child;
						hastimesig = true;
					}

					child = child.next_sibling();
				}
			}
		}
	}

	if (hasclef) {
		addClefLine(outdata, clefs, partdata, nowtime);
	}

	if (haskeysig) {
		addKeySigLine(outdata, keysigs, partdata, nowtime);
	}

	if (hastimesig) {
		addTimeSigLine(outdata, timesigs, partdata, nowtime);
	}

}



//////////////////////////////
//
// musicxml2hum_interface::addClefLine --
//

void musicxml2hum_interface::addClefLine(GridMeasure& outdata, 
		vector<xml_node>& clefs, vector<MxmlPart>& partdata, HumNum nowtime) {

	GridSlice* slice = new GridSlice(nowtime, SliceType::Clefs);
	outdata.push_back(slice);
	slice->initializePartStaves(partdata);

	for (int i=0; i<(int)partdata.size(); i++) {
		if (clefs[i]) {
			insertPartClefs(clefs[i], *slice->at(i));
		}
	}
}



//////////////////////////////
//
// musicxml2hum_interface::addTimeSigLine --
//

void musicxml2hum_interface::addTimeSigLine(GridMeasure& outdata, 
		vector<xml_node>& timesigs, vector<MxmlPart>& partdata, HumNum nowtime) {

	GridSlice* slice = new GridSlice(nowtime, SliceType::TimeSigs);
	outdata.push_back(slice);
	slice->initializePartStaves(partdata);

	for (int i=0; i<(int)partdata.size(); i++) {
		if (timesigs[i]) {
			insertPartTimeSigs(timesigs[i], *slice->at(i));
		}
	}
}



//////////////////////////////
//
// musicxml2hum_interface::addKeySigLine --
//

void musicxml2hum_interface::addKeySigLine(GridMeasure& outdata, 
		vector<xml_node>& keysigs, vector<MxmlPart>& partdata, HumNum nowtime) {

	GridSlice* slice = new GridSlice(nowtime, SliceType::KeySigs);
	outdata.push_back(slice);
	slice->initializePartStaves(partdata);

	for (int i=0; i<(int)partdata.size(); i++) {
		if (keysigs[i]) {
			insertPartKeySigs(keysigs[i], *slice->at(i));
		}
	}
}



//////////////////////////////
//
// musicxml2hum_interface::insertPartClefs --
//

void musicxml2hum_interface::insertPartClefs(xml_node clef, GridPart& part) {
	if (!clef) {
		// no clef for some reason.
		return;
	}

	HTp token;
	int staffnum = 0;
	while (clef) {
		clef = convertClefToHumdrum(clef, token, staffnum);
		part[staffnum]->setTokenLayer(0, token, 0);
	}
}



//////////////////////////////
//
// musicxml2hum_interface::insertPartKeySigs --
//

void musicxml2hum_interface::insertPartKeySigs(xml_node keysig, GridPart& part) {
	if (!keysig) {
		return;
	}

	HTp token;
	int staffnum = 0;
	while (keysig) {
		keysig = convertKeySigToHumdrum(keysig, token, staffnum);
		if (staffnum < 0) {
			// key signature applies to all staves in part (most common case)
			for (int s=0; s<(int)part.size(); s++) {
				if (s==0) {
					part[s]->setTokenLayer(0, token, 0);
				} else {
					HTp token2 = new HumdrumToken(*token);
					part[s]->setTokenLayer(0, token2, 0);
				}
			}
		} else {
			part[staffnum]->setTokenLayer(0, token, 0);
		}
	}
}



//////////////////////////////
//
// musicxml2hum_interface::insertPartTimeSigs --
//

void musicxml2hum_interface::insertPartTimeSigs(xml_node timesig, GridPart& part) {
	if (!timesig) {
		// no timesig
		return;
	}

	HTp token;
	int staffnum = 0;
	while (timesig) {
		timesig = convertTimeSigToHumdrum(timesig, token, staffnum);
		if (staffnum < 0) {
			// time signature applies to all staves in part (most common case)
			for (int s=0; s<(int)part.size(); s++) {
				if (s==0) {
					part[s]->setTokenLayer(0, token, 0);
				} else {
					HTp token2 = new HumdrumToken(*token);
					part[s]->setTokenLayer(0, token2, 0);
				}
			}
		} else {
			part[staffnum]->setTokenLayer(0, token, 0);
		}
	}
}



//////////////////////////////
//
//	musicxml2hum_interface::convertKeySigToHumdrum --
//
//  <key>
//     <fifths>4</fifths>
//

xml_node musicxml2hum_interface::convertKeySigToHumdrum(xml_node keysig,
		HTp& token, int& staffindex) {

	if (!keysig) {
		return keysig;
	}

	staffindex = -1;
	xml_attribute sn = keysig.attribute("number");
	if (sn) {
		staffindex = atoi(sn.value()) - 1;
	}

	int fifths = 0;

	xml_node child = keysig.first_child();
	while (child) {
		if (nodeType(child, "fifths")) {
			fifths = atoi(child.child_value());
		} 
		child = child.next_sibling();
	}

	stringstream ss;
	ss << "*k[";
	if (fifths > 0) {
		switch (fifths) {
			case 7: ss << "b#";
			case 6: ss << "e#";
			case 5: ss << "a#";
			case 4: ss << "d#";
			case 3: ss << "g#";
			case 2: ss << "c#";
			case 1: ss << "f#";
		}
	} else if (fifths < 0) {
		switch (fifths) {
			case 7: ss << "f-";
			case 6: ss << "g#";
			case 5: ss << "c#";
			case 4: ss << "d#";
			case 3: ss << "a#";
			case 2: ss << "e#";
			case 1: ss << "b#";
		}
	}
	ss << "]";

	token = new HumdrumToken(ss.str());

	keysig = keysig.next_sibling();
	if (!keysig) {
		return keysig;
	}
	if (nodeType(keysig, "key")) {
		return keysig;
	} else {
		return xml_node(NULL);
	}
}



//////////////////////////////
//
//	musicxml2hum_interface::convertTimeSigToHumdrum --
//
//  <time symbol="common">
//     <beats>4</beats>
//     <beat-type>4</beat-type>
//

xml_node musicxml2hum_interface::convertTimeSigToHumdrum(xml_node timesig,
		HTp& token, int& staffindex) {

	if (!timesig) {
		return timesig;
	}

	staffindex = -1;
	xml_attribute sn = timesig.attribute("number");
	if (sn) {
		staffindex = atoi(sn.value()) - 1;
	}

	int beats = -1;
	int beattype = -1;

	xml_node child = timesig.first_child();
	while (child) {
		if (nodeType(child, "beats")) {
			beats = atoi(child.child_value());
		} else if (nodeType(child, "beat-type")) {
			beattype = atoi(child.child_value());
		}
		child = child.next_sibling();
	}

	stringstream ss;
	ss << "*M" << beats<< "/" << beattype;
	token = new HumdrumToken(ss.str());

	timesig = timesig.next_sibling();
	if (!timesig) {
		return timesig;
	}
	if (nodeType(timesig, "time")) {
		return timesig;
	} else {
		return xml_node(NULL);
	}
}



//////////////////////////////
//
//	musicxml2hum_interface::convertClefToHumdrum --
//

xml_node musicxml2hum_interface::convertClefToHumdrum(xml_node clef,
		HTp& token, int& staffindex) {

	if (!clef) {
		// no clef for some reason.
		return clef;
	}

	staffindex = 0;
	xml_attribute sn = clef.attribute("number");
	if (sn) {
		staffindex = atoi(sn.value()) - 1;
	}

	string sign;
	int line = 0;

	xml_node child = clef.first_child();
	while (child) {
		if (nodeType(child, "sign")) {
			sign = child.child_value();
		} else if (nodeType(child, "line")) {
			line = atoi(child.child_value());
		}
		child = child.next_sibling();
	}

	// Check for percussion clefs, etc., here.
	stringstream ss;
	ss << "*clef" << sign << line;
	token = new HumdrumToken(ss.str());

	clef = clef.next_sibling();
	if (!clef) {
		return clef;
	}
	if (nodeType(clef, "clef")) {
		return clef;
	} else {
		return xml_node(NULL);
	}
}



//////////////////////////////
//
// musicxml2hum_interface::nodeType -- return true if node type matches
//     string.
//

bool musicxml2hum_interface::nodeType(xml_node node, const char* testname) {
	if (strcmp(node.name(), testname) == 0) {
		return true;
	} else {
		return false;
	}
}



//////////////////////////////
//
// musicxml2hum_interface::appendNullTokens --
//

void musicxml2hum_interface::appendNullTokens(HumdrumLine* line,
		MxmlPart& part) {
	int i;
	int staffcount = part.getStaffCount();
	int versecount = part.getVerseCount();
	for (i=staffcount-1; i>=0; i--) {
		line->appendToken(".");
	}
	for (i=0; i<versecount; i++) {
		line->appendToken(".");
	}
}



//////////////////////////////
//
// musicxml2hum_interface::getPartContent -- Extract the part elements in
//     the file indexed by part ID.
//

bool musicxml2hum_interface::getPartContent(
		map<string, xml_node>& partcontent,
		vector<string>& partids, xml_document& doc) {

	auto parts = doc.select_nodes("/score-partwise/part");
	int count = (int)parts.size();
	if (count != (int)partids.size()) {
		cerr << "Warning: part element count does not match part IDs count: "
		     << parts.size() << " compared to " << partids.size() << endl;
	}

	string partid;
	for (int i=0; i<(int)parts.size(); i++) {
		partid = getAttributeValue(parts[i], "id");
		if (partid.size() == 0) {
			cerr << "Warning: Part " << i << " has no ID" << endl;
		}
		auto status = partcontent.insert(make_pair(partid, parts[i].node()));
		if (status.second == false) {
			cerr << "Error: ID " << partids.back()
			     << " is duplicated and secondary part will be ignored" << endl;
		}
		if (find(partids.begin(), partids.end(), partid) == partids.end()) {
			cerr << "Error: Part ID " << partid
			     << " is not present in part-list element list" << endl;
			continue;
		}
	}

	if (partcontent.size() != partids.size()) {
		cerr << "Error: part-list count does not match part count "
		     << partcontent.size() << " compared to " << partids.size() << endl;
		return false;
	} else {
		return true;
	}
}



//////////////////////////////
//
// musicxml2hum_interface::getPartInfo -- Extract a list of the part ids,
//    and a reverse mapping to the <score-part> element to which is refers.
//
//	   part-list structure:
//        <part-list>
//          <score-part id="P1"/>
//          <score-part id="P2"/>
//          etc.
//        </part-list>
//

bool musicxml2hum_interface::getPartInfo(map<string, xml_node>& partinfo,
		vector<string>& partids, xml_document& doc) {
	auto scoreparts = doc.select_nodes("/score-partwise/part-list/score-part");
	partids.reserve(scoreparts.size());
	bool output = true;
	for (auto el : scoreparts) {
		partids.emplace_back(getAttributeValue(el.node(), "id"));
		auto status = partinfo.insert(make_pair(partids.back(), el.node()));
		if (status.second == false) {
			cerr << "Error: ID " << partids.back()
			     << " is duplicated and secondary part will be ignored" << endl;
		}
		output &= status.second;
		partinfo[partids.back()] = el.node();
	}
	return output;
}



//////////////////////////////
//
// musicxml2hum_interface::getChildElementText -- Return the (first)
//    matching child element's text content.
//

string musicxml2hum_interface::getChildElementText(xml_node root,
		const char* xpath) {
	return root.select_single_node(xpath).node().child_value();
}

string musicxml2hum_interface::getChildElementText(xpath_node root,
		const char* xpath) {
	return root.node().select_single_node(xpath).node().child_value();
}



//////////////////////////////
//
// musicxml2hum_interface::getAttributeValue -- For an xml_node, return
//     the value for the given attribute name.
//

string musicxml2hum_interface::getAttributeValue(xml_node xnode,
		const string& target) {
	for (auto at = xnode.first_attribute(); at; at = at.next_attribute()) {
		if (target == at.name()) {
			return at.value();
		}
	}
	return "";
}


string musicxml2hum_interface::getAttributeValue(xpath_node xnode,
		const string& target) {
	auto node = xnode.node();
	for (auto at = node.first_attribute(); at; at = at.next_attribute()) {
		if (target == at.name()) {
			return at.value();
		}
	}
	return "";
}



//////////////////////////////
//
// musicxml2hum_interface::printAttributes -- Print list of all attributes
//     for an xml_node.
//

void musicxml2hum_interface::printAttributes(xml_node node) {
	int counter = 1;
	for (auto at = node.first_attribute(); at; at = at.next_attribute()) {
		cout << "\tattribute " << counter++
		     << "\tname  = " << at.name()
		     << "\tvalue = " << at.value()
		     << endl;
	}
}

} // end namespace hum


