#include <string>
#include <iostream.h>

template <class T>
class StringTest {
public:
   virtual void runTest();
   void testFunction();
};

template <class T>
void StringTest<T>:: runTest() {
   testFunction ();
}

template <class T>
void StringTest <T>::testFunction() {
   // initialize s with string literal
   cout << "in StringTest" << endl;
   string s("I am a shot string");
   cout << s << endl;

   // insert 'r' to fix "shot"
   s.insert(s.begin()+10,'r' );
   cout << s << endl;

   // concatenate another string
   s += "and now a longer string";
   cout << s << endl;

   // find position where blank needs to be inserted
   string::size_type spos = s.find("and");
   s.insert(spos, " ");
   cout << s << endl;

   // erase the concatenated part
   s.erase(spos);
   cout << s << endl;
}

int main() {
   StringTest<wchar_t> ts;
   ts.runTest();
}

/* output:
I am a shot string
I am a short string
I am a short stringand now a longer string
I am a short string and now a longer string
I am a short string
*/
