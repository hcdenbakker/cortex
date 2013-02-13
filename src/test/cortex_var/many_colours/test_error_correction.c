/*
 * Copyright 2009-2011 Zamin Iqbal and Mario Caccamo
 * 
 * CORTEX project contacts:  
 * 		M. Caccamo (mario.caccamo@bbsrc.ac.uk) and 
 * 		Z. Iqbal (zam@well.ox.ac.uk)
 *
 * **********************************************************************
 *
 * This file is part of CORTEX.
 *
 * CORTEX is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * CORTEX is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with CORTEX.  If not, see <http://www.gnu.org/licenses/>.
 *
 * **********************************************************************
 */
/*
  test_error_correction.c
*/

#include <stdlib.h>

#include <CUnit.h>
#include <Basic.h>

#include "string_buffer.h"
#include "error_correction.h"
#include "file_reader.h"

void test_base_mutator()
{
  char* str="AACGT";
  StrBuf* strbuf=strbuf_new();
  strbuf_set(strbuf, str);
  CU_ASSERT(mutate_base(strbuf, 1, 0)==true);
  CU_ASSERT(strcmp(strbuf->buff, "ACCGT")==0);
  strbuf_set(strbuf, str);
  CU_ASSERT(mutate_base(strbuf, 1, 1)==true);
  CU_ASSERT(strcmp(strbuf->buff, "AGCGT")==0);
  strbuf_set(strbuf, str);
  CU_ASSERT(mutate_base(strbuf, 1, 2)==true);
  CU_ASSERT(strcmp(strbuf->buff, "ATCGT")==0);
  strbuf_set(strbuf, str);
  CU_ASSERT(mutate_base(strbuf, 1, 3)==false);

  strbuf_set(strbuf, str);
  CU_ASSERT(mutate_base(strbuf, 2, 0)==true);
  CU_ASSERT(strcmp(strbuf->buff, "AAAGT")==0);
  strbuf_set(strbuf, str);
  CU_ASSERT(mutate_base(strbuf, 2, 1)==true);
  CU_ASSERT(strcmp(strbuf->buff, "AAGGT")==0);
  strbuf_set(strbuf, str);
  CU_ASSERT(mutate_base(strbuf, 2, 2)==true);
  CU_ASSERT(strcmp(strbuf->buff, "AATGT")==0);
  strbuf_set(strbuf, str);
  CU_ASSERT(mutate_base(strbuf, 2, 3)==false);

  char* str2 = "NNAC";
  strbuf_set(strbuf, str2);
  CU_ASSERT(mutate_base(strbuf, 0, 0)==true);
  CU_ASSERT(strcmp(strbuf->buff, "ANAC")==0);
  strbuf_set(strbuf, str2);
  CU_ASSERT(mutate_base(strbuf, 0, 1)==true);
  CU_ASSERT(strcmp(strbuf->buff, "CNAC")==0);
  strbuf_set(strbuf, str2);
  CU_ASSERT(mutate_base(strbuf, 0, 2)==true);
  CU_ASSERT(strcmp(strbuf->buff, "GNAC")==0);
  strbuf_set(strbuf, str2);
  CU_ASSERT(mutate_base(strbuf, 0, 3)==true);
  CU_ASSERT(strcmp(strbuf->buff, "TNAC")==0);

}



void test_fix_end_if_unambiguous()
{

  if(NUMBER_OF_BITFIELDS_IN_BINARY_KMER > 1)
  {
    warn("Test not configured for NUMBER_OF_BITFIELDS_IN_BINARY_KMER > 1\n");
    return;
  }

  //first set up the hash/graph
  int kmer_size = 5;
  int number_of_bits = 10;
  int bucket_size = 10;

  //*********************************************
  // Test 1 - does it fix a kmer to match the graph 
  //          when there is a unique way of doing it
  //*********************************************

  dBGraph * db_graph = hash_table_new(number_of_bits, bucket_size, 10, kmer_size);
  
  //Load the following fasta: this is to be the trusted graph - just AAAAA
  // >
  // AAAAA


  int fq_quality_cutoff = 0;
  int homopolymer_cutoff = 0;
  boolean remove_duplicates_se = false;
  char ascii_fq_offset = 33;
  int into_colour = 0;

  unsigned int files_loaded = 0;
  unsigned long long bad_reads = 0, dup_reads = 0;
  unsigned long long seq_loaded = 0, seq_read = 0;

  load_se_filelist_into_graph_colour(
    "../data/test/error_correction/graph1.falist",
    fq_quality_cutoff, homopolymer_cutoff,
    remove_duplicates_se, ascii_fq_offset,
    into_colour, db_graph, 0, // 0 => falist/fqlist; 1 => colourlist
    &files_loaded, &bad_reads, &dup_reads, &seq_read, &seq_loaded,
    NULL, 0);


  StrBuf* readbuf = strbuf_new();
  StrBuf* kmerbuf = strbuf_init(5);
  char kmerstr[6];
  char* read1="TGACAAAAAC"; 
  strbuf_set(readbuf, read1);
  //at position 5 in the read we have AAAAC, which is fixable to AAAAA
  CU_ASSERT(fix_end_if_unambiguous(Right, readbuf, 5, kmerbuf, kmerstr, db_graph)==true);
  CU_ASSERT(strcmp(readbuf->buff, "TGACAAAAAA")==0);
  strbuf_set(readbuf, read1);//reset

  CU_ASSERT(fix_end_if_unambiguous(Left, readbuf, 5, kmerbuf, kmerstr, db_graph)==false);
  CU_ASSERT(strcmp(readbuf->buff, "TGACAAAAAC")==0);
  hash_table_free(&db_graph);
  strbuf_free(readbuf);
  strbuf_free(kmerbuf);




  //*********************************************
  // Test 2 - confirm it does not fix
  //          when there are two mutations which
  //          would make it in the graph
  //*********************************************

  db_graph = hash_table_new(number_of_bits, bucket_size, 10, kmer_size);
  
  //Load the following fasta: this is to be the trusted graph. note ACGGA can be corrected in TWO ways to matc this graph
  // >
  // ACGGCTTTACGGT


  load_se_filelist_into_graph_colour(
    "../data/test/error_correction/graph2.falist",
    fq_quality_cutoff, homopolymer_cutoff,
    remove_duplicates_se, ascii_fq_offset,
    into_colour, db_graph, 0, // 0 => falist/fqlist; 1 => colourlist
    &files_loaded, &bad_reads, &dup_reads, &seq_read, &seq_loaded,
    NULL, 0);


  readbuf = strbuf_new();
  kmerbuf = strbuf_init(5);

  char* read2="TGACACGGAGGTACGT";//contains ACGGA 
  strbuf_set(readbuf, read2);
  //at position 4 in the read we have ACGGA which is fixable to ACGGC OR ACGGT
  CU_ASSERT(fix_end_if_unambiguous(Right, readbuf, 4, kmerbuf, kmerstr, db_graph)==false);
  CU_ASSERT(strcmp(readbuf->buff, "TGACACGGAGGTACGT")==0);
  strbuf_set(readbuf, read2);//reset


  //in fact, now confirm nothing is changed anywhere in the read
  int j;
  for (j=0; j<9; j++)
    {
      CU_ASSERT(fix_end_if_unambiguous(Right, readbuf, j, kmerbuf, kmerstr, db_graph)==false);
      CU_ASSERT(strcmp(readbuf->buff, "TGACACGGAGGTACGT")==0);
      strbuf_set(readbuf, read2);//reset
    }


  hash_table_free(&db_graph);
  strbuf_free(readbuf);
  strbuf_free(kmerbuf);

  
}



void test_get_first_good_kmer_and_populate_qual_array()
{
  //pass in a read with chosen qualities, and check 
  //we correctly get int arrays describing which bases have good quals
  //and which kmers are in the graph
  //this function makes a performance choice, no abort and not bother
  //populating the kmer array as soon as we find out all the qualities are high,
  //as we immediately know we are not going to do any error-correction.


  //first set up the hash/graph
  int kmer_size = 5;
  int number_of_bits = 10;
  int bucket_size = 10;

  //*********************************************

  //*********************************************

  dBGraph * db_graph = hash_table_new(number_of_bits, bucket_size, 10, kmer_size);
  
  //Load the following fasta: this is to be the trusted graph
  // >
  // ACGGCTTTACGGT

  int fq_quality_cutoff = 0;
  int homopolymer_cutoff = 0;
  boolean remove_duplicates_se = false;
  char ascii_fq_offset = 33;
  int into_colour = 0;

  unsigned int files_loaded = 0;
  unsigned long long bad_reads = 0, dup_reads = 0;
  unsigned long long seq_loaded = 0, seq_read = 0;

  load_se_filelist_into_graph_colour(
    "../data/test/error_correction/graph3.falist",
    fq_quality_cutoff, homopolymer_cutoff,
    remove_duplicates_se, ascii_fq_offset,
    into_colour, db_graph, 0, // 0 => falist/fqlist; 1 => colourlist
    &files_loaded, &bad_reads, &dup_reads, &seq_read, &seq_loaded,
    NULL, 0);

  //read a small FASTQ - all these kmers ARE int he graph and have high quality
  // @zam all qual 30
  // ACGGCTTTACGGT
  // +
  // ?????????????


  SeqFile *sf = seq_file_open("../data/test/error_correction/fq_for_comparing_with_graph3.fq");
  if(sf == NULL)
    {
      // Error opening file
      fprintf(stderr, "Error: cannot read seq file '%s'\n", "../data/test/error_correction/fq_for_comparing_with_graph3.fq");
      exit(EXIT_FAILURE);
    }

  seq_next_read(sf);
  StrBuf* read_seq  = strbuf_new();
  StrBuf* read_qual = strbuf_new();
  seq_read_all_bases(sf, read_seq);
  seq_read_all_quals(sf, read_qual);
  int read_len = seq_get_length(sf);
  seq_file_close(sf);

  
  //now let's check
  int quals_good[read_len];
  set_int_array(quals_good, read_len, 1);


  char qual_cutoff=20+33;//33=ascii FASTQ sanger offset
  int first_good_kmer=-1;
  
  ReadCorrectionDecison result = get_first_good_kmer_and_populate_qual_array(read_seq, read_qual,
								   read_len-5+1, read_len,
								   quals_good,
								   qual_cutoff, &first_good_kmer,
								   db_graph, DontWorryAboutLowQualBaseUnCorrectable);
  CU_ASSERT(result==PrintUncorrected);
  
  int j;
  for (j=0; j<read_len; j++)
    {
      CU_ASSERT(quals_good[j]==1);
    }
  CU_ASSERT(first_good_kmer==-1);

  //result should be independent of policy
  set_int_array(quals_good, read_len, 1);
  first_good_kmer=-1;


  result = get_first_good_kmer_and_populate_qual_array(read_seq, read_qual,
						       read_len-5+1, read_len,
						       quals_good,
						       qual_cutoff, &first_good_kmer,
						       db_graph, DiscardReadIfLowQualBaseUnCorrectable);
  CU_ASSERT(result==PrintUncorrected);
  
  for (j=0; j<read_len; j++)
    {
      CU_ASSERT(quals_good[j]==1);
    }
  CU_ASSERT(first_good_kmer==-1);




  //now try again, but change the quality threshold so that all the qualities fail

  qual_cutoff=90+33;
  first_good_kmer=-1;
  set_int_array(quals_good, read_len, 1);
  result = get_first_good_kmer_and_populate_qual_array(read_seq, read_qual,
						       read_len-5+1, read_len,
						       quals_good,
						       qual_cutoff, &first_good_kmer,
						       db_graph, DontWorryAboutLowQualBaseUnCorrectable);
  CU_ASSERT(result==PrintCorrected); //this function does not know all kmers in the graph
  for (j=0; j<read_len; j++)
    {
      CU_ASSERT(quals_good[j]==0);
    }
  CU_ASSERT(first_good_kmer==0);


  first_good_kmer=-1;
  set_int_array(quals_good, read_len, 1);
  result = get_first_good_kmer_and_populate_qual_array(read_seq, read_qual,
					     read_len-5+1, read_len,
					     quals_good,
					     qual_cutoff, &first_good_kmer,
					     db_graph, DiscardReadIfLowQualBaseUnCorrectable);
  CU_ASSERT(result==PrintCorrected); 
  for (j=0; j<read_len; j++)
    {
      CU_ASSERT(quals_good[j]==0);
    }
  CU_ASSERT(first_good_kmer==0);




  //now try a more complicated read, with a kmer that is not in the graph, but HIGH quality
  //  @zam all qual 10 except30 at the base which isnot in the graph
  //  ACGGCTTGACGGT
  //  +
  //  +++++++?+++++

  //so at position 3 in the read we have GCTTG which is not in the graph, could be fixed to be in the graph,
  // BUT IS HIGH QUALITY, so should not be fixed. Upstream functions check that. This function just checks for presence.

  sf = seq_file_open("../data/test/error_correction/fq2_for_comparing_with_graph3.fq");
  if(sf == NULL)
    {
      // Error opening file
      fprintf(stderr, "Error: cannot read seq file '%s'\n", "../data/test/error_correction/fq2_for_comparing_with_graph3.fq");
      exit(EXIT_FAILURE);
    }

  seq_next_read(sf);
  seq_read_all_bases(sf, read_seq);
  seq_read_all_quals(sf, read_qual);
  read_len = seq_get_length(sf);
  seq_file_close(sf);

  first_good_kmer=-1;
  
  set_int_array(quals_good, read_len, 1);
  qual_cutoff=20+33;
  result = get_first_good_kmer_and_populate_qual_array(read_seq, read_qual,
					     read_len-5+1, read_len,
					     quals_good,
					     qual_cutoff, &first_good_kmer,
					     db_graph, DiscardReadIfLowQualBaseUnCorrectable);


  //NOTE - the following is correct behaviour.
  //this function just decides that it is worth trying to correct
  //as there is a kmer not in the graph. It doesn't check to see if that kmer has low quality
  CU_ASSERT(result==PrintCorrected);

  CU_ASSERT(quals_good[0]==0);
  CU_ASSERT(quals_good[1]==0);
  CU_ASSERT(quals_good[2]==0);
  CU_ASSERT(quals_good[3]==0);
  CU_ASSERT(quals_good[4]==0);
  CU_ASSERT(quals_good[5]==0);
  CU_ASSERT(quals_good[6]==0);
  CU_ASSERT(quals_good[7]==1);
  CU_ASSERT(quals_good[8]==0);
  CU_ASSERT(quals_good[9]==0);
  CU_ASSERT(quals_good[10]==0);
  CU_ASSERT(quals_good[11]==0);
  CU_ASSERT(quals_good[12]==0);

  CU_ASSERT(first_good_kmer==0);

  //and agin changing policy - should get same answer

  set_int_array(quals_good, read_len, 1);
  qual_cutoff=20+33;
  result = get_first_good_kmer_and_populate_qual_array(read_seq, read_qual,
					     read_len-5+1, read_len,
					     quals_good,
					     qual_cutoff, &first_good_kmer,
					     db_graph, DontWorryAboutLowQualBaseUnCorrectable);


  CU_ASSERT(result==PrintCorrected);

  CU_ASSERT(quals_good[0]==0);
  CU_ASSERT(quals_good[1]==0);
  CU_ASSERT(quals_good[2]==0);
  CU_ASSERT(quals_good[3]==0);
  CU_ASSERT(quals_good[4]==0);
  CU_ASSERT(quals_good[5]==0);
  CU_ASSERT(quals_good[6]==0);
  CU_ASSERT(quals_good[7]==1);
  CU_ASSERT(quals_good[8]==0);
  CU_ASSERT(quals_good[9]==0);
  CU_ASSERT(quals_good[10]==0);
  CU_ASSERT(quals_good[11]==0);
  CU_ASSERT(quals_good[12]==0);

  CU_ASSERT(first_good_kmer==0);







  //now shift the quality threshold to just above 30, so all the bases have low qual
  first_good_kmer=-1;
  
  set_int_array(quals_good, read_len, 1);
  qual_cutoff=31+33;
  result = get_first_good_kmer_and_populate_qual_array(read_seq, read_qual,
					     read_len-5+1, read_len,
					     quals_good,
					     qual_cutoff, &first_good_kmer,
					     db_graph, DontWorryAboutLowQualBaseUnCorrectable);


  CU_ASSERT(result==PrintCorrected);

  CU_ASSERT(quals_good[0]==0);
  CU_ASSERT(quals_good[1]==0);
  CU_ASSERT(quals_good[2]==0);
  CU_ASSERT(quals_good[3]==0);
  CU_ASSERT(quals_good[4]==0);
  CU_ASSERT(quals_good[5]==0);
  CU_ASSERT(quals_good[6]==0);
  CU_ASSERT(quals_good[7]==0);
  CU_ASSERT(quals_good[8]==0);
  CU_ASSERT(quals_good[9]==0);
  CU_ASSERT(quals_good[10]==0);
  CU_ASSERT(quals_good[11]==0);
  CU_ASSERT(quals_good[12]==0);

  CU_ASSERT(first_good_kmer==0);



  //and again with other policy
  first_good_kmer=-1;
  
  set_int_array(quals_good, read_len, 1);
  qual_cutoff=31+33;
  result = get_first_good_kmer_and_populate_qual_array(read_seq, read_qual,
					     read_len-5+1, read_len,
					     quals_good,
					     qual_cutoff, &first_good_kmer,
					     db_graph, DiscardReadIfLowQualBaseUnCorrectable);
  CU_ASSERT(result==PrintCorrected);

  CU_ASSERT(quals_good[0]==0);
  CU_ASSERT(quals_good[1]==0);
  CU_ASSERT(quals_good[2]==0);
  CU_ASSERT(quals_good[3]==0);
  CU_ASSERT(quals_good[4]==0);
  CU_ASSERT(quals_good[5]==0);
  CU_ASSERT(quals_good[6]==0);
  CU_ASSERT(quals_good[7]==0);
  CU_ASSERT(quals_good[8]==0);
  CU_ASSERT(quals_good[9]==0);
  CU_ASSERT(quals_good[10]==0);
  CU_ASSERT(quals_good[11]==0);
  CU_ASSERT(quals_good[12]==0);

  CU_ASSERT(first_good_kmer==0);





  //now shift the quality threshold to just below 10, so all bases have high qual
  //so now it doesn't matter about the kmers, whether they are in or out, you definitely
  //print the read uncorrected.
  
  set_int_array(quals_good, read_len, 1);
  qual_cutoff=9+33;
  first_good_kmer=-1;
  result = get_first_good_kmer_and_populate_qual_array(read_seq, read_qual,
					     read_len-5+1, read_len,
					     quals_good,
					     qual_cutoff, &first_good_kmer,
					     db_graph, DiscardReadIfLowQualBaseUnCorrectable);


  CU_ASSERT(result==PrintUncorrected);


  CU_ASSERT(quals_good[0]==1);
  CU_ASSERT(quals_good[1]==1);
  CU_ASSERT(quals_good[2]==1);
  CU_ASSERT(quals_good[3]==1);
  CU_ASSERT(quals_good[4]==1);
  CU_ASSERT(quals_good[5]==1);
  CU_ASSERT(quals_good[6]==1);
  CU_ASSERT(quals_good[7]==1);
  CU_ASSERT(quals_good[8]==1);
  CU_ASSERT(quals_good[9]==1);
  CU_ASSERT(quals_good[10]==1);
  CU_ASSERT(quals_good[11]==1);
  CU_ASSERT(quals_good[12]==1);

  //this isnot set- does not bother to check, since all quals high
  CU_ASSERT(first_good_kmer==-1);


  //now again with other policy
  set_int_array(quals_good, read_len, 1);
  qual_cutoff=9+33;
  first_good_kmer=-1;
  result = get_first_good_kmer_and_populate_qual_array(read_seq, read_qual,
					     read_len-5+1, read_len,
					     quals_good,
					     qual_cutoff, &first_good_kmer,
					     db_graph, DontWorryAboutLowQualBaseUnCorrectable);


  CU_ASSERT(result==PrintUncorrected);


  CU_ASSERT(quals_good[0]==1);
  CU_ASSERT(quals_good[1]==1);
  CU_ASSERT(quals_good[2]==1);
  CU_ASSERT(quals_good[3]==1);
  CU_ASSERT(quals_good[4]==1);
  CU_ASSERT(quals_good[5]==1);
  CU_ASSERT(quals_good[6]==1);
  CU_ASSERT(quals_good[7]==1);
  CU_ASSERT(quals_good[8]==1);
  CU_ASSERT(quals_good[9]==1);
  CU_ASSERT(quals_good[10]==1);
  CU_ASSERT(quals_good[11]==1);
  CU_ASSERT(quals_good[12]==1);

  //this isnot set- does not bother to check, since all quals high
  CU_ASSERT(first_good_kmer==-1);



  //now try a read where all kmers are not in the graph, but high quality
  //  @zam all qual 60, no kmers in graph
  //  ATATATATATAT
  //  +
  //  ]]]]]]]]]]]]

  //this read should be printed uncorrected, as all high quality

  sf = seq_file_open("../data/test/error_correction/fq3_for_comparing_with_graph3.fq");
  if(sf == NULL)
    {
      // Error opening file
      fprintf(stderr, "Error: cannot read seq file '%s'\n", "../data/test/error_correction/fq3_for_comparing_with_graph3.fq");
      exit(EXIT_FAILURE);
    }

  seq_next_read(sf);
  seq_read_all_bases(sf, read_seq);
  seq_read_all_quals(sf, read_qual);
  read_len = seq_get_length(sf);
  seq_file_close(sf);

  first_good_kmer=-1;  
  set_int_array(quals_good, read_len, 1);
  qual_cutoff=10+33;
  result = get_first_good_kmer_and_populate_qual_array(read_seq, read_qual,
					     read_len-5+1, read_len,
					     quals_good,
					     qual_cutoff, &first_good_kmer,
					     db_graph, DontWorryAboutLowQualBaseUnCorrectable);

  CU_ASSERT(result==PrintUncorrected);

  CU_ASSERT(quals_good[0]==1);
  CU_ASSERT(quals_good[1]==1);
  CU_ASSERT(quals_good[2]==1);
  CU_ASSERT(quals_good[3]==1);
  CU_ASSERT(quals_good[4]==1);
  CU_ASSERT(quals_good[5]==1);
  CU_ASSERT(quals_good[6]==1);
  CU_ASSERT(quals_good[7]==1);
  CU_ASSERT(quals_good[8]==1);
  CU_ASSERT(quals_good[9]==1);
  CU_ASSERT(quals_good[10]==1);
  CU_ASSERT(quals_good[11]==1);

  //not set
  CU_ASSERT(first_good_kmer==-1);


  //policy makes no difference:
  first_good_kmer=-1;  
  set_int_array(quals_good, read_len, 1);
  qual_cutoff=10+33;
  result = get_first_good_kmer_and_populate_qual_array(read_seq, read_qual,
					     read_len-5+1, read_len,
					     quals_good,
					     qual_cutoff, &first_good_kmer,
					     db_graph, DiscardReadIfLowQualBaseUnCorrectable);

  CU_ASSERT(result==PrintUncorrected);

  CU_ASSERT(quals_good[0]==1);
  CU_ASSERT(quals_good[1]==1);
  CU_ASSERT(quals_good[2]==1);
  CU_ASSERT(quals_good[3]==1);
  CU_ASSERT(quals_good[4]==1);
  CU_ASSERT(quals_good[5]==1);
  CU_ASSERT(quals_good[6]==1);
  CU_ASSERT(quals_good[7]==1);
  CU_ASSERT(quals_good[8]==1);
  CU_ASSERT(quals_good[9]==1);
  CU_ASSERT(quals_good[10]==1);
  CU_ASSERT(quals_good[11]==1);

  //not set
  CU_ASSERT(first_good_kmer==-1);





  //now try a read where all kmers are not in the graph, and NOT all high quality
  // @zam all qual 60, except one with low quality 3, but no kmers in graph
  // ATATATATATAT
  //+
  //]]]]]]]]]]]#




  sf = seq_file_open("../data/test/error_correction/fq4_for_comparing_with_graph3.fq");
  if(sf == NULL)
    {
      // Error opening file
      fprintf(stderr, "Error: cannot read seq file '%s'\n", "../data/test/error_correction/fq4_for_comparing_with_graph3.fq");
      exit(EXIT_FAILURE);
    }

  seq_next_read(sf);
  seq_read_all_bases(sf, read_seq);
  seq_read_all_quals(sf, read_qual);
  read_len = seq_get_length(sf);
  seq_file_close(sf);

  first_good_kmer=-1;  
  set_int_array(quals_good, read_len, 1);
  qual_cutoff=10+33;
  result = get_first_good_kmer_and_populate_qual_array(read_seq, read_qual,
					     read_len-5+1, read_len,
					     quals_good,
					     qual_cutoff, &first_good_kmer,
					     db_graph, DontWorryAboutLowQualBaseUnCorrectable);



  CU_ASSERT(result==PrintUncorrected);

  CU_ASSERT(quals_good[0]==1);
  CU_ASSERT(quals_good[1]==1);
  CU_ASSERT(quals_good[2]==1);
  CU_ASSERT(quals_good[3]==1);
  CU_ASSERT(quals_good[4]==1);
  CU_ASSERT(quals_good[5]==1);
  CU_ASSERT(quals_good[6]==1);
  CU_ASSERT(quals_good[7]==1);
  CU_ASSERT(quals_good[8]==1);
  CU_ASSERT(quals_good[9]==1);
  CU_ASSERT(quals_good[10]==1);
  CU_ASSERT(quals_good[11]==0);

  //not set - did not find a good kmer
  CU_ASSERT(first_good_kmer==-1);


  //this time the policy makes a difference

  first_good_kmer=-1;  
  set_int_array(quals_good, read_len, 1);
  qual_cutoff=10+33;
  result = get_first_good_kmer_and_populate_qual_array(read_seq, read_qual,
					     read_len-5+1, read_len,
					     quals_good,
					     qual_cutoff, &first_good_kmer,
					     db_graph, DiscardReadIfLowQualBaseUnCorrectable);



  CU_ASSERT(result==Discard);

  CU_ASSERT(quals_good[0]==1);
  CU_ASSERT(quals_good[1]==1);
  CU_ASSERT(quals_good[2]==1);
  CU_ASSERT(quals_good[3]==1);
  CU_ASSERT(quals_good[4]==1);
  CU_ASSERT(quals_good[5]==1);
  CU_ASSERT(quals_good[6]==1);
  CU_ASSERT(quals_good[7]==1);
  CU_ASSERT(quals_good[8]==1);
  CU_ASSERT(quals_good[9]==1);
  CU_ASSERT(quals_good[10]==1);
  CU_ASSERT(quals_good[11]==0);

  //not set also
  CU_ASSERT(first_good_kmer==-1);





  hash_table_free(&db_graph);
  strbuf_free(read_seq);
  strbuf_free(read_qual);





}


void test_error_correct_file_against_graph()
{
  //simple test, with one good kmer in middle. All low qual. Check error corrects left and right.
  // include a check of the stats collection


  //first set up the hash/graph
  int kmer_size = 5;
  int number_of_bits = 10;
  int bucket_size = 10;

  //*********************************************

  //*********************************************

  dBGraph * db_graph = hash_table_new(number_of_bits, bucket_size, 10, kmer_size);
  
  //Load the following fasta: this is to be the trusted graph
  // >
  // ACGGCTTTACGGT



  int fq_quality_cutoff = 0;
  int homopolymer_cutoff = 0;
  boolean remove_duplicates_se = false;
  char ascii_fq_offset = 33;
  int into_colour = 0;

  unsigned int files_loaded = 0;
  unsigned long long bad_reads = 0, dup_reads = 0;
  unsigned long long seq_loaded = 0, seq_read = 0;

  load_se_filelist_into_graph_colour(
    "../data/test/error_correction/graph3.falist",
    fq_quality_cutoff, homopolymer_cutoff,
    remove_duplicates_se, ascii_fq_offset,
    into_colour, db_graph, 0, // 0 => falist/fqlist; 1 => colourlist
    &files_loaded, &bad_reads, &dup_reads, &seq_read, &seq_loaded,
    NULL, 0);


  //now test this fastq
  // @only one kmer in graph, CTTTA
  // CCCCCTTTAAAAT
  // +
  // #############


  //we will start at CTTTA and move right, error correcting, and then go back to CTTTA and move left
  //
  //graph      ACGGCTTTACGGT
  // read      CCCCCTTTAAAAT
  // step1     ccccctttaCaat  << correct base at posn 9
  // step2     ccccctttaCGat  << correct base at posn 10
  // step3     ccccctttaCGGt << correct base at posn 11. 
  //                           < no need to correct posn 12
  // step4     cccGctttaCGGt  << correct base at posn 3
  // step5     ccGGctttaCGGt  << correct base at posn 2
  //                          no need to correct posn 1
  // step6     AcGGctttaCGGt  << correct base at posn 0


  char quality_cutoff= 10;
  char ascii_qual_offset = 33;
  char* outfile = "../data/test/error_correction/fq5_for_comparing_with_graph3.err_corrected.fa";
  int bases_modified_count_array[20];
  int posn_modified_count_array[20];
  int bases_modified_count_array_size=20;
  int i;
  for (i=0; i<20; i++)
    {
      bases_modified_count_array[i]=0;
      posn_modified_count_array[i]=0;
    }
  error_correct_file_against_graph("../data/test/error_correction/fq5_for_comparing_with_graph3.fq", 
				   quality_cutoff, ascii_qual_offset,
				   db_graph, outfile,
				   bases_modified_count_array,
				   posn_modified_count_array,
				   bases_modified_count_array_size,
				   DontWorryAboutLowQualBaseUnCorrectable);

  CU_ASSERT(bases_modified_count_array[0]==0);
  CU_ASSERT(bases_modified_count_array[1]==0);
  CU_ASSERT(bases_modified_count_array[2]==0);
  CU_ASSERT(bases_modified_count_array[3]==0);
  CU_ASSERT(bases_modified_count_array[4]==0);
  CU_ASSERT(bases_modified_count_array[5]==0);
  CU_ASSERT(bases_modified_count_array[6]==1);
  CU_ASSERT(bases_modified_count_array[7]==0);
  CU_ASSERT(bases_modified_count_array[8]==0);
  CU_ASSERT(bases_modified_count_array[9]==0);
  CU_ASSERT(bases_modified_count_array[10]==0);
  CU_ASSERT(bases_modified_count_array[11]==0);
  CU_ASSERT(bases_modified_count_array[12]==0);
  CU_ASSERT(bases_modified_count_array[13]==0);
  CU_ASSERT(bases_modified_count_array[14]==0);

  CU_ASSERT(posn_modified_count_array[0]==1);
  CU_ASSERT(posn_modified_count_array[1]==0);
  CU_ASSERT(posn_modified_count_array[2]==1);
  CU_ASSERT(posn_modified_count_array[3]==1);
  CU_ASSERT(posn_modified_count_array[4]==0);
  CU_ASSERT(posn_modified_count_array[5]==0);
  CU_ASSERT(posn_modified_count_array[6]==0);
  CU_ASSERT(posn_modified_count_array[7]==0);
  CU_ASSERT(posn_modified_count_array[8]==0);
  CU_ASSERT(posn_modified_count_array[9]==1);
  CU_ASSERT(posn_modified_count_array[10]==1);
  CU_ASSERT(posn_modified_count_array[11]==1);
  CU_ASSERT(posn_modified_count_array[12]==0);


  SeqFile *sf = seq_file_open(outfile);
  if(sf == NULL)
    {
      // Error opening file
      fprintf(stderr, "Error: cannot read seq file '%s'\n", outfile);
      exit(EXIT_FAILURE);
    }

  seq_next_read(sf);
  StrBuf* read_seq  = strbuf_new();
  seq_read_all_bases(sf, read_seq);
  seq_file_close(sf);


  //graph is this          ACGGCTTTACGGT
  // input data is this    CCCCCTTTAAAAT
  // correction should be  ACGGCTTTACGGT
  //  i get                ACGGCTTTACGGT 

  CU_ASSERT(strcmp(read_seq->buff, "ACGGCTTTACGGT")==0);



  //same, but make sure does not fix high qual.
}
