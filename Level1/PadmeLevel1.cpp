#include <stdlib.h>
#include <stdio.h>
#include <vector>

#include "ADCBoard.hh"
#include "DBService.hh"
#include "RootIO.hh"

int main(int argc, char* argv[])
{
  
  int c;
  std::string listfile = "";
  std::string dbfile = "db/PadmeDAQ.db";
  std::string outfile = "rawdata.root";
  std::string datadir = "data";
  int runnr = 0;
  int verbose = 0;

  // Parse options
  while ((c = getopt (argc, argv, "d:r:l:o:v:h")) != -1) {
    switch (c)
      {
      case 'r':
	if (runnr!=0) {
          fprintf (stderr, "Error while processing option '-r'. Multiple runs specified.\n");
          exit(1);
	}
        if ( sscanf(optarg,"%d",&runnr) != 1 ) {
          fprintf (stderr, "Error while processing option '-r'. Wrong parameter '%s'.\n", optarg);
          exit(1);
        }
        if (runnr<0) {
          fprintf (stderr, "Error while processing option '-r'. Run number set to %d (must be >=0).\n", runnr);
          exit(1);
        }
        fprintf(stdout,"Merging files from run %d\n",runnr);
        break;
      case 'd':
        datadir = optarg;
        fprintf(stdout,"Set input data directory to '%s'\n",datadir.c_str());
        break;
      case 'o':
        outfile = optarg;
        fprintf(stdout,"Set output data file to '%s'\n",outfile.c_str());
        break;
      case 'l':
        listfile = optarg;
        fprintf(stdout,"Data will be read from files listed in '%s'\n",listfile.c_str());
        break;
      case 'v':
        if ( sscanf(optarg,"%d",&verbose) != 1 ) {
          fprintf (stderr, "Error while processing option '-v'. Wrong parameter '%s'.\n", optarg);
          exit(1);
        }
        if (verbose<0) {
          fprintf (stderr, "Error while processing option '-v'. Verbose level set to %d (must be >=0).\n", verbose);
          exit(1);
        }
        fprintf(stdout,"Set verbose level to %d\n",verbose);
        break;
      case 'h':
        fprintf(stdout,"\nPadmeDigi ([-r run_number]|[-l list_file]) [-o output root file] [-v verbosity] [-h]\n\n");
        fprintf(stdout,"  -r: define run to process\n");
        fprintf(stdout,"  -l: define file with list of data files to process\n");
        fprintf(stdout,"  -d: define directory where input files are located\n");
        fprintf(stdout,"  -o: define an output file in root format\n");
        fprintf(stdout,"  -v: define verbose level\n");
        fprintf(stdout,"  -h: show this help message and exit\n\n");
        exit(0);
      case '?':
        if (optopt == 'v') {
          // verbose with no argument: just enable at minimal level
          verbose = 1;
          break;
        } else if (optopt == 'i' || optopt == 'l' || optopt == 'o')
          fprintf (stderr, "Option -%c requires an argument.\n", optopt);
        else if (isprint(optopt))
          fprintf (stderr, "Unknown option `-%c'.\n", optopt);
        else
          fprintf (stderr,"Unknown option character `\\x%x'.\n",optopt);
        exit(1);
      default:
        abort();
      }
  }

  // Verify that some input was specified
  if ( listfile.compare("")==0 && runnr==0 ) {
    fprintf (stderr,"No run number and no list file were specified. Exiting.");
    exit(1);
  }
  if ( listfile.compare("")!=0 && runnr!=0 ) {
    fprintf (stderr,"Both run number and list file were specified. Exiting.");
    exit(1);
  }

  ADCBoard* board;
  std::vector<ADCBoard*> boards;

  // If no list is specified, get board and file lists from DB
  if (listfile.compare("")==0) {

    // Initialize DB connection
    DBService* db = DBService::GetInstance();
    db->SetDBFilePath(dbfile);
    db->Connect();

    // Get from DB list of board ids used in current run
    std::vector<int> boardList;
    int rc = db->GetBoardList(boardList,runnr);
    if (rc != DBSERVICE_OK) {
      printf("ERROR retrieving from DB list of files for run %d. Aborting\n",runnr);
      exit(1);
    }

    // Create vector of boards
    for(unsigned int b=0; b<boardList.size(); b++) {

      printf("Board %d\tReading data from board id %d\n",b,boardList[b]);
      board = new ADCBoard(boardList[b]);

      // Get list of files created for each board during run
      std::vector<std::string> fileList;
      rc = db->GetFileList(fileList,runnr,boardList[b]);
      if (rc != DBSERVICE_OK) {
	printf("ERROR retrieving from DB list of files for run %d board id %d. Aborting\n",runnr,boardList[b]);
	exit(1);
      }
      // Add files to board
      for(unsigned int f=0; f<fileList.size(); f++) {
	std::string filePath = datadir+"/"+fileList[f];
	printf("\tFile %d - Reading from file %s\n",f,filePath.c_str());
	board->AddFile(filePath);
      }

      // Add board to boards vector
      boards.push_back(board);
      
    }

    // Disconnect from DB
    db->Disconnect();

  } else {

    // Get list of boards and files from file
    ifstream list;
    std::string line;
    int bid;
    char bfile[1024];
    list.open(listfile.c_str());
    while(!list.eof()){

      getline(list,line);
      if (line.compare("")!=0) {

	if ( sscanf(line.c_str(),"%d %s",&bid,bfile) != 2 ) {
	  printf("ERROR while parsing list '%s'. Aborting\n",listfile.c_str());
	  exit(1);
	}

	// Find board with correct board id or add it to list if new
	std::vector<ADCBoard*>::iterator it;
	for (it = boards.begin(); it != boards.end(); ++it) {
	  board = *it;
	  if (board->GetBoardId() == bid) break;
	}
	if (it == boards.end()) {
	  printf("Board id %d\n",bid);
	  board = new ADCBoard(bid);
	  boards.push_back(board);
	}

	// Add file to board
	std::string filePath = datadir+"/"+bfile;
	printf("\tBoard %d - File %s\n",bid,filePath.c_str());
	board->AddFile(filePath);

      }

    }
    list.close();

  }

  // Show list of known boards/files
  printf("Reading %d board(s)\n",(int)boards.size());
  std::vector<ADCBoard*>::iterator it;
  for (it = boards.begin(); it != boards.end(); ++it) {
    board = *it;
    printf("Board %d Files %d\n",board->GetBoardId(),board->GetNFiles());
    for(int f=0; f<board->GetNFiles(); f++) printf("File %d %s\n",f,board->GetFileName(f).c_str());
  }
  
  // Connect to root services
  RootIO* root = new RootIO();

  // Initialize root output file
  if ( root->Init(outfile) != 0 ) {
    printf("ERROR while initializing root output to file '%s'. Aborting\n",outfile.c_str());
    exit(1);
  }

  // Loop over all events in files

  UInt_t TT0[boards.size()]; // Initial time tag
  UInt_t TT[boards.size()]; // Current time tag

  int eventnr = 0;
  while(1){

    // Load next event for all boards
    unsigned int nEOR = 0;
    for(unsigned int b=0; b<boards.size(); b++) {
      if ( boards[b]->NextEvent() == 0 ) {
	nEOR++;
	printf("Board %d has reached end of run.\n",boards[b]->GetBoardId());
      }
    }
    if (nEOR != 0) {
      if (nEOR == boards.size()) {
	printf("All boards reached end of run.\n");
      } else {
	printf("WARNING: only %d board(s) reached end of run.\n",nEOR);
      }
      break;
    }

    for(unsigned int b=0; b<boards.size(); b++) {
      if(eventnr==0){
	TT0[b] =  boards[b]->Event()->GetTriggerTimeTag(0);
	TT[b] =  TT0[b];
	printf("Board %1d NEv %8u Tabs %f (0x%08x)\n",b,boards[b]->Event()->GetEventCounter(),TT0[b]*8.5E-9,TT0[b]);
      }else{
	UInt_t tt = boards[b]->Event()->GetTriggerTimeTag(0);
	Int_t dt = tt-TT[b];
	if (dt<0) { dt += (1<<30); }
	if (b==0) {
	  printf("Board %1d NEv %8u Tabs %f (0x%08x) Dt %f (0x%08x)\n",b,boards[b]->Event()->GetEventCounter(),tt*8.5E-9,tt,dt*8.5E-9,dt);
	} else {
	  Int_t dt0 = tt-TT[0];
	  printf("Board %1d NEv %8u Tabs %f (0x%08x) Dt %f (0x%08x) DtB0 %f (0x%08x)\n",b,boards[b]->Event()->GetEventCounter(),tt*8.5E-9,tt,dt*8.5E-9,dt,dt0*8.5E-9,dt0);
	}
	TT[b] = tt;
      }
    }
    // The event is complete: copy structure to TRawEvent and send to output file
    root->FillRawEvent(runnr,eventnr,boards);

    // Count event
    eventnr++;
    //if (eventnr>5000) break;
  }

  // Finalize root file
  root->Exit();

  // End of run procedure
  printf("File %s for run %d closed after writing %d events\n",outfile.c_str(),runnr,eventnr);

  exit(0);
}