// 2002-05-13

namespace __gnu_test
{
  enum 	region { oriental, egyptian, greek, etruscan, roman };

  // Test one.
  class gnu_obj_1
  {
  protected:
    typedef region antiquities;
    const bool 		test;
    const int 		key1;
    long       		key2;

    antiquities 	value;

  public:
    gnu_obj_1(antiquities a, long l): test(true), key1(5), key2(l), value(a) {}
  };

  // Test two.
  template<typename T>
    class gnu_obj_2: public virtual gnu_obj_1
    {
    protected:
      antiquities	value_derived;
      
    public:
      gnu_obj_2(antiquities b): gnu_obj_1(oriental, 7), value_derived(b) { }
    }; 

  // Test three.
  template<typename T>
    class gnu_obj_3
    {
    protected:
      typedef region antiquities;
      gnu_obj_2<int>   	data;
      
    public:
      gnu_obj_3(antiquities b): data(etruscan) { }
    }; 
} 

int main()
{
  using namespace __gnu_test;
  gnu_obj_1		test1(egyptian, 4589);
  gnu_obj_2<long>	test2(roman);
  gnu_obj_3<long>	test3(greek);
  return 0;
}
