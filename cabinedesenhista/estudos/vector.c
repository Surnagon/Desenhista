	//~ int j, k;
	//~ int ROWS = 0, COLS = 0;
	//~ using vec = std::vector<std::string>;
	//~ using matrix = std::vector<vec>;
	//~ vec linha;
	//~ matrix matriz;
	//~ std::vector<int> > linha;
	//~ std::vector<linha> > matrix;
	//~ std::ifstream file("cartesianas.csv");
	
	//~ std::string line, val;

	//~ while( getline( file, row ) )
	//~ {
		//~ ROWS++;
		//~ std::stringstream ss( row );
		//~ while ( getline ( ss, item, ',' ) ) linha.push_back( item );
		//~ matriz.push_back( linha );
	//~ }

	//~ while( std::getline( file, line ) ) {
		//~ if ( !file.good() )
			//~ break;

		//~ std::stringstream iss(line);

		//~ while ( std::getline ( iss, val, ',' ) )
		//~ {
			//~ if ( !iss.good() )
				//~ break;
			
			//~ std::stringstream convertor(val);
			//~ linha.push_back( val.str() );
		//~ }
		//~ matriz.push_back( linha );
	//~ }
	
	//~ for(int i=0;i<ROWS*COLS;i++) {
		//~ for(int j=0; j<ROWS*COLS; j++) {
			//~ if(polar[j+1].arr[a] < polar[j].arr[a])
			//~ {
				//~ std::swap(matrix[j+1].arr[0], polar[j].arr[0]);
				//~ std::swap(matrix[j+1].arr[1], polar[j].arr[1]);
				//~ std::swap(matrix[j+1].arr[2], polar[j].arr[2]);
			//~ } else if ((polar[j+1].arr[a] == polar[j].arr[a]) && (polar[j+1].arr[b] < polar[j].arr[b])) {
				//~ std::swap(polar[j+1].arr[0], polar[j].arr[0]);
				//~ std::swap(polar[j+1].arr[1], polar[j].arr[1]);
				//~ std::swap(polar[j+1].arr[2], polar[j].arr[2]);
			//~ }
		//~ }
    //~ }
