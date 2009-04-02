#include <test_dB_graph.h>
#include <test_file_reader.h>
#include <test_cyclic_count.h>
#include <CUnit.h>
#include <Basic.h>

int  main()
{

  CU_pSuite pSuite = NULL;

  /* initialize the CUnit test registry */
  if (CUE_SUCCESS!=CU_initialize_registry())
    return CU_get_error();
  
  /* add a suite to the registry */
  pSuite = CU_add_suite("Suite_1", NULL, NULL);
  if (NULL == pSuite) {
    CU_cleanup_registry();
    return CU_get_error();
  }

  /* add the tests to the suite */

  
  if (NULL == CU_add_test(pSuite, "test hash_table_find for dB_graphs",  test_hash_table_find)){
    CU_cleanup_registry();
    return CU_get_error();
  }

  if (NULL == CU_add_test(pSuite, "test supernode walking",  test_supernode_walking)){
    CU_cleanup_registry();
    return CU_get_error();
  }

  if (NULL == CU_add_test(pSuite, "test writting/reading graph",  test_writing_reading_graph)){
    CU_cleanup_registry();
    return CU_get_error();
  }

  if (NULL == CU_add_test(pSuite, "test writting/reading binary node",  test_writing_reading_graph)){
    CU_cleanup_registry();
    return CU_get_error();
  }

   if (NULL == CU_add_test(pSuite, "test dumping/reading binary",  test_dump_load_binary)){
    CU_cleanup_registry();
    return CU_get_error();
  }

   if (NULL == CU_add_test(pSuite, "test calculation of N50",  test_get_N50)){
    CU_cleanup_registry();
    return CU_get_error();
  }

   if (NULL == CU_add_test(pSuite, "test function for rotating/shifting binary kmers",  test_rotate)){
    CU_cleanup_registry();
    return CU_get_error();
  }

  /* Run all tests using the CUnit Basic interface */
  CU_basic_set_mode(CU_BRM_VERBOSE);
  CU_basic_run_tests();
 
  CU_cleanup_registry();
  return CU_get_error();


}





