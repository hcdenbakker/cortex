/*
  dB_graph_population.c - implementation
 */

#include <stdlib.h>
#include <stdio.h>

#include <element.h>
#include <hash_table.h>
#include <dB_graph.h>
#include <dB_graph_population.h>
#include <pqueue_pop.h>
#include <seq.h>
#include <string.h>



boolean db_node_is_supernode_end(dBNode * element,Orientation orientation, EdgeArrayType edge_type, int edge_index, dBGraph* db_graph)
{
  if (element==NULL)
    {
      //printf("Sending null pointer to db_node_is_supernode_end\n");
      return false;
    }
  char edges = get_edge_copy(*element, edge_type, edge_index);
  
  if (orientation == reverse)
    {
      //shift along so the 4 most significant bits become the 4 least - we've passed out argument by copy so not altering the original
      edges >>= 4;
    }

  edges &= 15; // AND with 00001111 so that we only look at the 4 least significant bits 
   
  //is supernode end EITHER if it has 0 edges out
  if (edges == 0)
    {
      return true;
    }
  else if ((edges != 1) && (edges != 2) && (edges != 4) && (edges != 8))  // or if it has too many edges, ie >1
    {
      return true;
    }
  else  //next node has more than one arrow in
    {
      Orientation next_orientation;
      
      //we know this element has only one edge out. What is it? The function db_node_has_precisely_one_edge fills the answer into argument 3
      Nucleotide nuc, reverse_nuc;
      if (db_node_has_precisely_one_edge(element, orientation, &nuc, edge_type, edge_index))
	{

	  dBNode* next_node = db_node_get_next_node_for_specific_person_or_pop(element, orientation, &next_orientation, nuc, &reverse_nuc,db_graph, edge_type, edge_index);

	  if ( (next_node==element) && (next_orientation==orientation) )
	    {
	      return true; //infinite self-loop
	    }
	  else if (db_node_has_precisely_one_edge(next_node, opposite_orientation(next_orientation), &nuc, edge_type, edge_index))
	    {
	      return false;//successor node also has only one edge coming in
	    }
	  else //successor has multiple edges
	    {
	      return true;      
	    }
	}
    }

  //to keep compiler happy
  exit(1);
  return true;
}

dBNode * db_node_get_next_node_for_specific_person_or_pop(dBNode * current_node, Orientation current_orientation, 
			       Orientation * next_orientation,
			       Nucleotide edge, Nucleotide * reverse_edge,HashTable * db_graph, EdgeArrayType type, int index){
  
  BinaryKmer kmer = element_get_kmer(current_node);
  dBNode * next_node;
  BinaryKmer rev_kmer = binary_kmer_reverse_complement(kmer,db_graph->kmer_size);
  
  if (current_orientation == reverse){   
    *reverse_edge = binary_kmer_get_last_nucleotide(kmer);
    kmer = rev_kmer;
  }
  else{
    *reverse_edge = binary_kmer_get_last_nucleotide(rev_kmer);
  }


  //Mario - we are going to get confused by things like this. here edge variable is a nucleotide,
  //but elsewhere edge is th char that describes all the edges in a node.
  kmer = binary_kmer_add_nucleotide_shift(kmer,edge, db_graph->kmer_size);

  //get node from table 
  next_node =  db_graph_find_node_restricted_to_specific_person_or_population(element_get_key(kmer,db_graph->kmer_size),db_graph,type,index);

  if (next_node != NULL){
    *next_orientation = db_node_get_orientation(kmer,next_node,db_graph->kmer_size);
  }

  return next_node;
}









// wrapper for hash_table_find, which allows you to look in the hash table
// specifically for nodes related to a specific person or population
// person or population  specified by which edge array type
// which person or pop specified by index

dBNode *  db_graph_find_node_restricted_to_specific_person_or_population(Key key, dBGraph * hash_table, EdgeArrayType type, int index)

{

  dBNode *  e = hash_table_find(key, hash_table);

  //ASSUMING read length is strictly greater than kmer-length
  //then you should never see a kmer (node) which is unconnected to another (even if the other is itself)
  //ie to check if this kmer is seen in a given person/pop, it is enough to check if it has an edge

  Edges edge_for_this_person_or_pop = get_edge_copy(*e, type, index);

  if (edge_for_this_person_or_pop == 0)
    {
      return NULL;
    }
  else
    {
      return e;
    }
  
}


//WARNING: this marks as visited any node that it walks over (and is in the supernode)
char * get_seq_from_elem_to_end_of_supernode_for_specific_person_or_pop(dBNode * node, Orientation orientation, dBGraph * db_graph, boolean * is_cycle, EdgeArrayType type, int index){
  Nucleotide nucleotide1, nucleotide2, rev_nucleotide;
  char * seq = NULL;
  int max = 1500;
  Orientation original_orientation, next_orientation;
  dBNode * original_node;
  dBNode * next_node;
  int seq_length = 0;
  
  

  seq = malloc(1500*sizeof(char)); 

  if (seq == NULL){
    puts("dB_graph: cannot assign memory for sequence\n");
    exit(1);
  }

  original_node = node;
  original_orientation = orientation; 
  

  //Mark the element we're starting at as visited
  db_node_set_status(node,visited);

  *is_cycle = false;

  
  while(db_node_has_precisely_one_edge(node,orientation,&nucleotide1, type, index)) {
 
    if ((seq_length+1)>max){
      max+=1000;
      seq = (char*)realloc(seq, sizeof(char) * max);
    }

    if (seq == NULL){
      printf("dB_graph: cannot assign memory\n");
      exit(1);
    }

   
    next_node =  db_node_get_next_node_for_specific_person_or_pop(node,orientation,&next_orientation,nucleotide1,&rev_nucleotide,db_graph, type, index);
    
    if(next_node == NULL){
      printf("dB_graph: didnt find node in hash table: %s\n", binary_kmer_to_seq(element_get_kmer(node),db_graph->kmer_size));
      exit(1);
    }	         
    
    if (DEBUG){
      printf("TRY TO ADD %c - next node %s\n",binary_nucleotide_to_char(nucleotide1),
	     next_orientation == forward ? binary_kmer_to_seq(element_get_kmer(next_node),db_graph->kmer_size) :  binary_kmer_to_seq(binary_kmer_reverse_complement(element_get_kmer(next_node),db_graph->kmer_size),db_graph->kmer_size));
	     

    }
    
    //check for multiple entry edges 
    if (db_node_has_precisely_one_edge(next_node,opposite_orientation(next_orientation),&nucleotide2, type, index)){
      seq[seq_length] =  binary_nucleotide_to_char(nucleotide1);
      seq_length++;
      if (DEBUG){
	printf("ADD %c\n",binary_nucleotide_to_char(nucleotide1));
      }
    }
    else{
      if (DEBUG){
	printf("Multiple entries\n");
      }
      break;
    }
    
    
    //loop
    if ((next_node == original_node) && (next_orientation == original_orientation)){      
      *is_cycle = true;
      //remove last addition
      seq_length--;
      break;
    }
      
    db_node_set_status(next_node,visited);
    
    node = next_node;
    orientation = next_orientation;      
  }

  //clever thing about this next line is that if it turns out that the very first node that you handed in as an argument
  //had an edge back to itself - so it was a cycle, then the first character in seq is \0 and when the person who calls this prints what you return, they print nothing
  seq[seq_length] = '\0';
  return seq;
}


void print_supernode_for_specific_person_or_pop(HashTable* db_graph, dBNode * node,EdgeArrayType type, int index, boolean is_for_testing, char** for_test, int* index_for_test)
{

  FILE * fout;
  long count=0;
  
  char filename [200];
  if (count % 100000000 == 0)
    {
      int num = count / 100000000;
      
      if (count !=0)
	{
	  fclose(fout);
	}

      if (type == individual_edge_array)
	{
	  sprintf(filename,"out_nodes_kmer_%i_person_%i_subset_%i",db_graph->kmer_size,index,num);
	}
      else
	{
	  sprintf(filename,"out_nodes_kmer_%i_population_%i_subset_%i",db_graph->kmer_size,index,num);
	}
      //fprintf(stderr,"opening file %s\n",filename);
      fout = fopen(filename,"w");
    }
  
  db_graph_print_supernode_for_specific_person_or_pop(fout,node,db_graph, type,index, is_for_testing,  for_test,index_for_test);
 


}


// *********************************************
// functions applied to a person/pop's graph
// *********************************************


void db_graph_traverse_specific_person_or_pop(void (*f)(HashTable*, Element *, EdgeArrayType, int, boolean, char**, int*),HashTable * hash_table, EdgeArrayType type, int index, boolean is_for_testing, char** for_test, int* index_for_test){

  int i;
  for(i=0;i<hash_table->number_buckets;i++){
    pqueue_traverse_specific_person_or_pop(f,hash_table, &(hash_table->table[i]), type,index, is_for_testing, for_test, index_for_test);
  }

}

//will print only nodes for specific person/population

// ******NOTE****** if you want to go on to print for other people, you need to set status to none for all the nodes again after going thrrough the whole
// graph printing supernodes - otherwise some will be left marked visited.

//if you pass in true for the boolean is_for_testing,  then instead of printing to the file, it adds the supernode to the array of char*'s 
//this enables the caller to sort the supernodes lexicographically, and test that the right supernodes are being found
//this is not scalable to a ful genome, but sufficient for detailed unit tests

void db_graph_print_supernode_for_specific_person_or_pop(FILE * file, dBNode * node, dBGraph * db_graph, EdgeArrayType type, int index, boolean is_for_testing, char** for_test, int* index_for_test ){


  //don't do anything if this node does not occur in the graph for this person/population
  if (!db_node_is_this_node_in_this_person_or_populations_graph(node, type, index))
    {
      return;
    }

  char * seq = NULL;
  char * seq_forward = NULL;
  char * seq_reverse = NULL; 
  char * seq_reverse_reversed = NULL;
  int i;
  int length_reverse = 0;
  boolean is_cycle_forward, is_cycle_reverse;

  //printf("PRINT SUPERNODE for person %d\n", index);
  if (! db_node_check_status(node,visited) && ! db_node_check_status(node,pruned))
    {
      
      //this function binary_kmer_to_seq does the malloc for what it returns.
      seq = binary_kmer_to_seq(element_get_kmer(node),db_graph->kmer_size);
      
      if (DEBUG){
      printf("\nSTART Supernode %s\n",seq);    
      printf("go forward\n");
      }
      
      //compute the forward path until the end of the supernode
      //mark the nodes in the path as visited.
 
     //return is_cycle_forward == true if the path closes a loop
 
      //this function mallocs the sequence that it returns, and will always initialise the is_cycle_forward variable, that is currently uninitialised
      //note that if going forward there is a cycle, then seq_forward will contain garbage, so don't use it. 
     seq_forward = get_seq_from_elem_to_end_of_supernode_for_specific_person_or_pop(node,forward,db_graph,&is_cycle_forward, type, index);
      
     if (DEBUG){
       printf("Z NODE c %s\n",seq); 
       if (is_cycle_forward)
	 {
	   printf("NODE f - none since is cycle forward\n");
	 }
       else
	 {
	   printf("NODE f %s\n",seq_forward);
	 }
     }
      
      if (! is_cycle_forward)
	{
    	  if (DEBUG){
	  printf("go reverse\n");
	  }
	  
	  //compute the reverse path...
	  //this function mallocs the sequence that it returns, andwill always initialise the is_cycle_reverse variable, that is currently uninitialised
	  //printf("getting rev seq for person %d", index);
	  seq_reverse = get_seq_from_elem_to_end_of_supernode_for_specific_person_or_pop(node,reverse,db_graph,&is_cycle_reverse, type, index);
	  
	  if (is_cycle_reverse){
	    puts("cycle in the reverse orientation without cycle in the forward orientation - impossible!\n");
	    exit(1);
	  }
	
	  if (DEBUG)
	    {
	      printf("Z NODE r %s\n",seq_reverse);
	    }
	  
	  length_reverse = strlen(seq_reverse);
	}

      seq_reverse_reversed = malloc((length_reverse+1)*sizeof(char));


      if (seq_reverse_reversed==NULL)
	{
	  printf("Cannot mallo cseq rev rev");
	  exit(1);
	}
      
      
      //reverse the reverse sequence
      for(i=0;i<length_reverse;i++){
	seq_reverse_reversed[i] = reverse_char_nucleotide(seq_reverse[length_reverse-i-1]);
	//printf("ZAMZAM seq rev rev next char is %c\n", seq_reverse_reversed[i]);
      }
      seq_reverse_reversed[length_reverse]='\0';
      //so if we had a cycle in the forward/reverse directions, this would only contain the single character \0

      if (DEBUG){
	if (length_reverse>0)
	  {
	    printf("Z NODE rr %s\n", seq_reverse_reversed);
	  }
      }
      
      //printf(">NODE\n%s%s%s\n",seq_reverse_reversed,seq,seq_forward); 
      
      if (!is_for_testing) 
	{
	  fprintf(file,">NODE\n%s%s%s\n",seq_reverse_reversed,seq,seq_forward); 
	}
      else
	{
	  int length_of_supernode = strlen(seq_reverse_reversed)+strlen(seq) +strlen(seq_forward) ;
	  if (length_of_supernode==0)
	    {
	      printf("Null supernode");
	      exit(1);
	    }


	  //assume the called has malloced space for output

	  for_test[*index_for_test] = (char*) calloc(length_of_supernode+1,sizeof(char));
	  if (for_test[*index_for_test]==NULL)
	    {
	      printf("Unable to calloc for supernode");
	      exit(1);
	    }
	  for_test[*index_for_test][0]='\0';
	  strcat(for_test[*index_for_test],seq_reverse_reversed);
	  strcat(for_test[*index_for_test],seq);
	  strcat(for_test[*index_for_test],seq_forward);
	  for_test[*index_for_test][length_of_supernode]='\0';
	  
	  //Now make sure you are using the smaller of the sequence and its rev complement
	  BinaryKmer snode_kmer = seq_to_binary_kmer(for_test[*index_for_test],length_of_supernode);
	  BinaryKmer rev_snode_kmer =  binary_kmer_reverse_complement(snode_kmer, length_of_supernode);
	  
	  if (rev_snode_kmer<snode_kmer)
	    {
	      free(for_test[*index_for_test]);
	      for_test[*index_for_test] = binary_kmer_to_seq(rev_snode_kmer,length_of_supernode);
	    }
	  
	  (*index_for_test)++;

	}


      free(seq);
      free(seq_forward);
      free(seq_reverse);
      free(seq_reverse_reversed);
	  
    }
  else{
    if (DEBUG){
      if ( db_node_check_status(node,visited)){
	printf("\n%s: visited\n", binary_kmer_to_seq(element_get_kmer(node),db_graph->kmer_size));
      }
      if ( db_node_check_status(node,pruned)){
	printf("\n%s: pruned\n", binary_kmer_to_seq(element_get_kmer(node),db_graph->kmer_size));
      }
    }
  }


}




void db_graph_set_all_visited_nodes_to_status_none_for_specific_person_or_population(dBGraph* hash_table, EdgeArrayType type, int index)
{
  printf("not implemented yet");
  exit(1);
}




dBNode* db_graph_get_first_node_in_supernode_containing_given_node_for_specific_person_or_pop(dBNode* node, EdgeArrayType type, int index, dBGraph* db_graph)
{
  
  if (! (db_node_is_this_node_in_this_person_or_populations_graph(node, type, index)))
    {
      if (node==NULL)
	{
	  //printf("Bloody node is null so of course cant get first node");
	}
      else
	{
	  //printf("This person %d does not have this node %s\n", index, binary_kmer_to_seq(node->kmer, db_graph->kmer_size));
	}
      return NULL;
    }

  if (db_node_check_status(node,pruned))
    {
      //don't waste time with pruned nodes.
      //printf("zam0.6\n");

      return NULL;
    }
  
  //printf("zam1\n");
  boolean is_cycle;
  Nucleotide nucleotide1, nucleotide2, rev_nucleotide;
  Orientation original_orientation, next_orientation, orientation;
  dBNode * original_node=node;
  dBNode * next_node;


  //First node in supernode is, almost by definition, what you get if you go in the Reverse direction (with respect to the Node)
  // as far as you can go.
  original_orientation = reverse; 
  orientation = reverse;
  is_cycle = false;


  while(db_node_has_precisely_one_edge(node,orientation,&nucleotide1, type, index)) {
 

    next_node =  db_node_get_next_node_for_specific_person_or_pop(node,orientation,&next_orientation,nucleotide1,&rev_nucleotide,db_graph, type, index);
    
    if(next_node == NULL){
      printf("dB_graph: didnt find node in hash table: %s\n", binary_kmer_to_seq(element_get_kmer(node),db_graph->kmer_size));
      exit(1);
    }	         

    if (DEBUG)
      {
      printf("TRY TO ADD %c - next node %s\n",binary_nucleotide_to_char(nucleotide1),
	     next_orientation == forward ? binary_kmer_to_seq(element_get_kmer(next_node),db_graph->kmer_size) :  
	     binary_kmer_to_seq(binary_kmer_reverse_complement(element_get_kmer(next_node),db_graph->kmer_size),db_graph->kmer_size));
	     
      }
    

    //check for multiple entry edges 
    if (db_node_has_precisely_one_edge(next_node,opposite_orientation(next_orientation),&nucleotide2, type, index))
      {
      }
    else
      {
	if (DEBUG)
	  {
	    printf("Multiple entries\n");
	  }
	//break;
	 
	//printf("returning this first nodem, with kmer %s\n", binary_kmer_to_seq(node->kmer, db_graph->kmer_size));
      return node; //we have gone as far as we can go - the next node has multiple entries. So we are now at the first node of the supernode
      }
    
    
    //loop
    if ((next_node == original_node) && (next_orientation == original_orientation))
      {      

	is_cycle = true;
	//break;
	
	//printf("We have a loop, so original node will do, with kmer %s\n", binary_kmer_to_seq(original_node->kmer, db_graph->kmer_size));
	return original_node; //we have a loop that returns to where we start. Might as well consider ourselves as at the fiurst node of the supernode right at the beginning
      }
    
    //printf("zam8\n");
    node = next_node;
    orientation = next_orientation;      
  }
  //printf("zam9 - we have found the first node, it is %s\n", binary_kmer_to_seq(node->kmer, db_graph->kmer_size));
  return node;

}


dBNode* db_graph_get_next_node_in_supernode_for_specific_person_or_pop(dBNode* node, Orientation orientation, Orientation* next_orientation, EdgeArrayType type, int index, dBGraph* db_graph)
{
  if (! (db_node_is_this_node_in_this_person_or_populations_graph(node, type, index)))
    {
      //printf("\nThis node is not in the graph of this person\n");
      return NULL;
    }
  else if (db_node_check_status(node,pruned))
    {
      printf("ignore pruned node");
      //don't waste time with pruned nodes.
      return NULL;
    }
  else if (db_node_is_supernode_end(node, orientation, type, index, db_graph))
    {
      if (DEBUG)
	{
	  printf("this node is at the end of the supernode, in this orientation, so cant return the next one\n");
	}
      return NULL;
    }

  Nucleotide nucleotide_for_only_edge, reverse_nucleotide_for_only_edge;

  
  db_node_has_precisely_one_edge(node,orientation,&nucleotide_for_only_edge, type, index);//gives us nucleotide
  
  dBNode* next_node =  db_node_get_next_node_for_specific_person_or_pop(node,orientation ,next_orientation,nucleotide_for_only_edge,&reverse_nucleotide_for_only_edge, db_graph, type, index);
  
  if(next_node == NULL){
    printf("dB_graph: didnt find node in hash table: %s\n", binary_kmer_to_seq(element_get_kmer(node),db_graph->kmer_size));
    exit(1);
  }	         

  if (DEBUG)
    {
      printf("TRY TO ADD %c - next node %s\n",binary_nucleotide_to_char(nucleotide_for_only_edge),
	     next_orientation == forward ? binary_kmer_to_seq(element_get_kmer(next_node),db_graph->kmer_size) :  
	     binary_kmer_to_seq(binary_kmer_reverse_complement(element_get_kmer(next_node),db_graph->kmer_size),db_graph->kmer_size));
      
    }
    

  //check for multiple entry edges 
  Nucleotide nuc;
  if (db_node_has_precisely_one_edge(next_node,opposite_orientation(*next_orientation),&nuc, type, index))
    {
    }
  else
    {
      //double check
      if (node ==NULL)
	{
	  printf("programming error. returning null node when my model in my head says impossible\n");
	  exit(1);
	}
      return node; //we have gone as far as we can go - the next node has multiple entries. So we are now at the first node of the supernode
    }
    
    
  //loop
  if ((next_node == node) && (*next_orientation == orientation))
    {      
      //double check
      if (node ==NULL)
	{
          printf("programming error. returning null node when my model in my head says impossible\n");
	  exit(1);
        }

	return node; //we have a kmer that loops back on itself
    }
  
  return next_node;

}


// Given a node, will look at each person's supernode containing that node. 
// For each of these supernodes, independently,  find the longest contiguous subsection that has min people-coverage > min_covg_for_pop_supernode.
// (Note this may not contain the original node.)
// Then compare across people, and define your consensus as the sub_supernode which has the most people backing it.
// If two people have equally good sub_supernodes, choose that of the person with highest index.
// Pass in pre-malloced Sequence* for answer

void  db_graph_find_population_consensus_supernode_based_on_given_node(Sequence* pop_consensus_supernode, dBNode* node, int min_covg_for_pop_supernode, int min_length_for_pop_supernode, 
								       dBGraph* db_graph)
{
  int length_of_best_sub_supernode_in_each_person[NUMBER_OF_INDIVIDUALS_PER_POPULATION];
  int index_of_start_of_best_sub_supernode_in_each_person[NUMBER_OF_INDIVIDUALS_PER_POPULATION];

  int i;
  for (i=0; i< NUMBER_OF_INDIVIDUALS_PER_POPULATION; i++)
    {
      length_of_best_sub_supernode_in_each_person[i]=0;
      index_of_start_of_best_sub_supernode_in_each_person[i]=0;
    }

  for (i=0; i< NUMBER_OF_INDIVIDUALS_PER_POPULATION; i++)
    {
        //get the first node in the supernode for this edge, within this person't graph
      dBNode* first_node = db_graph_get_first_node_in_supernode_containing_given_node_for_specific_person_or_pop(node, individual_edge_array, i, db_graph);
      
      //note that node may be null, and also may not be in this person's graph. In this case, ignore.
      if (first_node ==NULL)
	{
	  //printf("first node for %d is nul, so ignore \n",i);
	  index_of_start_of_best_sub_supernode_in_each_person[i]=0;
	  length_of_best_sub_supernode_in_each_person[i]=0;
	}
      else
	{
	  //this returns 0 in length_of_best_sub_supernode_in_each_person if no subsupernode matches constraints
	  //printf("\nFind best sub supernode in person %d based on first node in supernode, which is %s\n", i, binary_kmer_to_seq(first_node->kmer, db_graph->kmer_size) );
	  db_graph_get_best_sub_supernode_given_min_covg_and_length_for_specific_person_or_pop(first_node,  &index_of_start_of_best_sub_supernode_in_each_person[i], &length_of_best_sub_supernode_in_each_person[i], 
											       min_covg_for_pop_supernode,  min_length_for_pop_supernode, individual_edge_array, i, db_graph); 
	  //printf("OK - that sub supernode in person %d starts at %d ends at %d\n", i, index_of_start_of_best_sub_supernode_in_each_person[i], length_of_best_sub_supernode_in_each_person[i]);
	}
    }

  
  //check that at least one person has a subsupernode that matches criteria
  boolean noone_has_decent_sub_supernode=true;
  for (i=0; (i< NUMBER_OF_INDIVIDUALS_PER_POPULATION) && (noone_has_decent_sub_supernode==true); i++)
    {
      if ( length_of_best_sub_supernode_in_each_person[i] >0 )
	{
	  noone_has_decent_sub_supernode=false;
	}
    }
  
  if (noone_has_decent_sub_supernode)
    {
      pop_consensus_supernode->seq[0]='\0';
      //printf("noone has a decent supernode\n");
      return;
    }


  //Now find which person has the bext sub_supernode nucleated at this node 
  int person_with_best_sub_supernode;
  int max=0;
  for (i=0; i< NUMBER_OF_INDIVIDUALS_PER_POPULATION; i++)
    {
      if ( length_of_best_sub_supernode_in_each_person[i] > max )
	{
	  person_with_best_sub_supernode=i;
	  max = length_of_best_sub_supernode_in_each_person[i];
	}
    }

  //printf("OKOKOK we think the person qwith =best snode is %d and length is %d\n",  person_with_best_sub_supernode, length_of_best_sub_supernode_in_each_person[person_with_best_sub_supernode]); 
  if (max==0)
    {
      printf("This should be impossible. Max size of sub supernode over all people is 0. Since we start wth a kmer that is in the graph, at least one person ought to have it\n");
      exit(1);
    }
  
  else
    {
      if (max > pop_consensus_supernode->max)
	{
	  printf("pop_consensus_supernode not big enough - only alloced %d and we need %d\n", pop_consensus_supernode->max, max);
	  exit(1);
	}


      int start = index_of_start_of_best_sub_supernode_in_each_person[person_with_best_sub_supernode];
      int end =    start+ length_of_best_sub_supernode_in_each_person[person_with_best_sub_supernode]-1;
      if ((start<0) || (end<0))
	{
	  printf("This is wrong. start is %d and end is %d for person %d and their length is %d\n", start,end, person_with_best_sub_supernode, length_of_best_sub_supernode_in_each_person[person_with_best_sub_supernode]);
	  exit(1);
	}      
      //      if (start==end)
      //	{
      //	  //then the best we have anaged is a single kmer. No need to bother getting that subsection
      //	  pop_consensus_supernode->seq = ???
      //	}
  
      else if (db_graph_get_subsection_of_supernode_containing_given_node_as_sequence(pop_consensus_supernode->seq, node, start, end, individual_edge_array, person_with_best_sub_supernode, db_graph)==1)
	{
	  printf("Aborting - something wrong with gettig the subsection");
	  exit(1);
	}
      //printf("OKOK found bvest subsection for is %s\n", pop_consensus_supernode->seq);
    }

  

}

//subsection (argument1) is allocated by the caller
//returns 0 if successfully, 1 otherwise
int db_graph_get_subsection_of_supernode_containing_given_node_as_sequence(char* subsection, dBNode* node, int start, int end, EdgeArrayType type, int index, dBGraph* db_graph)
{
  //printf("CALL GET SUBSECTION With start %d and end %d\n\n", start, end);
  if ( (start<0) || (end<0) || (end-start<0) )
    {
      if (DEBUG)
	{
	  printf("bad args for getting subsection start %d and end %d", start, end);
	}
      subsection="ERROR";
      return 1;
    }

  
  dBNode* first_node = db_graph_get_first_node_in_supernode_containing_given_node_for_specific_person_or_pop(node, type, index, db_graph);
  dBNode* current_node=first_node;
  dBNode* next_node;
  Orientation correct_direction_to_go;

  if (db_node_is_supernode_end(first_node,forward, type, index, db_graph))
    {
      correct_direction_to_go=reverse;
    }
  else if (db_node_is_supernode_end(first_node,reverse, type, index, db_graph))
    {
      correct_direction_to_go=forward; 
    }
  else
    {
      printf("Something has gone wrong. This node has been given as first node of a supernode, but db_node_is_supernode_end thinks it is not in either direction\n");
      return 1;
    }

  

 int i;
 Orientation current_orientation, next_orientation;
 current_orientation = correct_direction_to_go;

  for (i=0; i<start; i++)
    {
      next_node = db_graph_get_next_node_in_supernode_for_specific_person_or_pop(current_node, current_orientation, &next_orientation, type, index, db_graph);
      if (next_node==NULL)
	{
	  if (DEBUG)
	    {
	      printf("You're asking for the section from node %d to node %d in supernode that is not even that long", start, end);
	    }
	  return 1;
	}
      current_node=next_node;
      current_orientation=next_orientation;
    }

  char* first_kmer_in_subsection; 
  //first kmer depends on which direction you start in with respect to the first node OF THE SUBSECTION, not the first node of the supernode
  if (current_orientation == forward) //i.e. orientation of first node of subsection
    {
      first_kmer_in_subsection = binary_kmer_to_seq(current_node->kmer, db_graph->kmer_size);
    }
  else
    {
      first_kmer_in_subsection = binary_kmer_to_seq(   binary_kmer_reverse_complement(current_node->kmer, db_graph->kmer_size) , db_graph->kmer_size);
    }
  

   for(i=0; i<db_graph->kmer_size; i++)
   {
      subsection[i] = first_kmer_in_subsection[i];
   }
   free(first_kmer_in_subsection);

   for (i=db_graph->kmer_size; i<=db_graph->kmer_size + end-start-1; i++)
   {
      next_node = db_graph_get_next_node_in_supernode_for_specific_person_or_pop(current_node, current_orientation, &next_orientation, type, index, db_graph);
      if (next_node==NULL)
        {
	  if (DEBUG)
	    {
	      printf("Youre asking for the section from node %d to node %d in supernode that is not even that long", start, end);
	    }
          return 1;
        }
      Nucleotide nuc_for_next_edge;
      db_node_has_precisely_one_edge(current_node,current_orientation,&nuc_for_next_edge, type, index);
      subsection[i]=binary_nucleotide_to_char(nuc_for_next_edge);

      current_node=next_node;
      current_orientation=next_orientation;

      
   }
   subsection[db_graph->kmer_size + end-start]='\0';

   return 0;
}


// walk through the supernode, from one end to the other. As soon as we hit a node that has people-covg >= min_covg_for_pop_supernode
// then note down the coordinate (how far through the supernode), and keep going until we meet a node with people-covg < min_covg_for_pop_supernode
// Work out the length of the chunk you found, which had sufficient covg. If > min_length_for_pop_supernode, then you have found your first
// candidate for this person's best sub-supernode. Log the start-point and length. Then continue walkign through the supernode until you
// find another node that has people_covg sufficiently high. repeat the process for this point, and if its candidate is longer than our
// current candidate, then replace it with this new one. etc.

//Note that if none of the nodes match criteria, return 0 in length of best sub_supernode
void  db_graph_get_best_sub_supernode_given_min_covg_and_length_for_specific_person_or_pop(dBNode* first_node_in_supernode,  int* index_for_start_of_sub_supernode,
                                                                                           int* length_of_best_sub_supernode, int min_people_coverage,
                                                                                           int min_length_of_sub_supernode, EdgeArrayType type, int index, dBGraph* db_graph)
{

  if (first_node_in_supernode==NULL)
    {
      printf("do not give null pointer to get_best_sub_supernode function");
      exit(1);
    }

  // OK - which direction do we walk?
  // Either - this node is a singleton supernode (either totally unconnected or too many edges in both directions)
  // Or    - we can go forward/reverse only, because the other direction has 0 or >1 edges.
  // Any other possibility breaks the condition that we are at a supernode end.

  Orientation correct_direction_to_go;

  if (db_node_is_supernode_end(first_node_in_supernode,forward, type, index, db_graph))
    {
    correct_direction_to_go=reverse;
    }
  else if (db_node_is_supernode_end(first_node_in_supernode,reverse, type,  index, db_graph))
    {
      correct_direction_to_go=forward; 
    }
  else
    {
      //must therefore be the case that this node is not in this person's graph. Check this
      if ( ! (db_node_is_this_node_in_this_person_or_populations_graph(first_node_in_supernode, type, index)) )
	{
	  *index_for_start_of_sub_supernode=0;
	  *length_of_best_sub_supernode=0;
	}
      else
	{
	  printf("Programming error. This node is supposed to be at the start of a supernode for this person, but is not in either direction. However it IS in their graph\n");
	}
    }


  
  int start_of_best_section_so_far=0;
  int length_of_best_section_so_far=0;
  boolean in_middle_of_a_good_section=false;
  int current_start=0;
  int current=0;

  dBNode* current_node=first_node_in_supernode;
  // printf("starting at first node in supernode: %s\n", binary_kmer_to_seq(current_node->kmer,db_graph->kmer_size));
  dBNode* next_node;
  Orientation current_orientation = correct_direction_to_go;
  Orientation next_orientation;

  boolean reached_end=false;

  //does the start_node have enough coverage?

  if (element_get_number_of_people_or_pops_containing_this_element(current_node, type, index) < min_people_coverage)
    {
    }
  else
    {
      in_middle_of_a_good_section=true;      
    }
  while(!reached_end)
    {
      next_node=db_graph_get_next_node_in_supernode_for_specific_person_or_pop(current_node, current_orientation, &next_orientation, type, index, db_graph);
      if (next_node==NULL)
	{
	  //printf("Reached end of supernode\n");
	  reached_end=true;
	  continue;
	}
      else if (next_node==first_node_in_supernode) 
	{
	  //printf("reached end of supernode - in this case back at the start so it stops just before becoming a loop\n");
	  reached_end=true;
	  continue;
	}


      int next_people_cov= element_get_number_of_people_or_pops_containing_this_element(next_node, type, index);

      if  ( next_people_cov < min_people_coverage)
	{

	  current++;
	  current_node=next_node;
	  current_orientation=next_orientation;
	  current_start=current;
	  in_middle_of_a_good_section=false;

	  //rest of this if clause should be DEBUG only
	  char* next_kmer;
	  if (next_orientation==forward)
	    {
	      next_kmer = binary_kmer_to_seq(next_node->kmer,db_graph->kmer_size);
	    }
	  else
	    {
	      next_kmer=binary_kmer_to_seq( binary_kmer_reverse_complement(next_node->kmer,db_graph->kmer_size), db_graph->kmer_size );
	    }
	  
	  
	  //printf("Looking for best subsection Next node is %s\n", next_kmer);
	  //printf("Too little peope coverage on this node - only %d\n", next_people_cov);
	  
	 
	}
      
      else //there is a next node, and it has enough people coverage
	{

	  current++;
          current_node=next_node;
          current_orientation=next_orientation;

	  if (in_middle_of_a_good_section)
	    {
	      
	    }
	  else
	    {
	      current_start=current;
	      in_middle_of_a_good_section=true;
	    }

	  //printf("Looking for best subsection Next node is %s\n", binary_kmer_to_seq(next_node->kmer,db_graph->kmer_size));
	
	  if (current-current_start+1 > length_of_best_section_so_far)
	    {
	      start_of_best_section_so_far=current_start;
	      length_of_best_section_so_far = current-current_start+1; //remember is length in nodes not bases
	    }
	  //printf("People covg is sufficient, at %d\nCurrent is %d, current_start is %d, best length sofar s %d\n", next_people_cov, current, current_start, length_of_best_section_so_far);
	  
	}
    }

  //length of best section is counted in supernodes. min length of sub supernode is in bases
  if ( ( length_of_best_section_so_far + db_graph->kmer_size - 1) >= min_length_of_sub_supernode)
    {
      //printf("Good. store best section start as %d, and length %d", start_of_best_section_so_far, length_of_best_section_so_far);
      *index_for_start_of_sub_supernode=start_of_best_section_so_far;
      *length_of_best_sub_supernode=length_of_best_section_so_far;
      return;
    }
  else
    {
      //printf("This person has nothing. Min length is %d but we only manage %d\n", min_length_of_sub_supernode, length_of_best_section_so_far);
      *index_for_start_of_sub_supernode=0;
      *length_of_best_sub_supernode=0;
      return;
    }
}

