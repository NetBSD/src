@@
expression V;
@@

- V =
  isc_mempool_create(...);
- assert_int_equal(V, ISC_R_SUCCESS);

@@
expression V;
@@

- V =
  isc_mempool_create(...);
- check_result(V, ...);

@@
@@

- CHECK(
  isc_mempool_create(...)
- )
  ;

@@
@@

- RUNTIME_CHECK(
  isc_mempool_create(...)
- == ISC_R_SUCCESS)
  ;

@@
expression V;
statement S;
@@

- V =
  isc_mempool_create(...);
- if (V != ISC_R_SUCCESS) S

@@
statement S;
@@

- if (
  isc_mempool_create(...)
- != ISC_R_SUCCESS) S
+ ;
