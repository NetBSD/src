// GNU C++ "extern" instantiation declarations for basic_string.

extern template class string_char_traits <__INST>;
extern template class basic_string <__INST>;

extern template basic_string<__INST> operator+ (const basic_string<__INST>&, const basic_string<__INST>&);
extern template basic_string<__INST> operator+ (const __INST*, const basic_string<__INST>&);
extern template basic_string<__INST> operator+ (__INST, const basic_string<__INST>&);
extern template basic_string<__INST> operator+ (const basic_string<__INST>&, const __INST*);
extern template basic_string<__INST> operator+ (const basic_string<__INST>&, __INST);
extern template bool operator== (const basic_string<__INST>&, const basic_string<__INST>&);
extern template bool operator== (const __INST*, const basic_string<__INST>&);
extern template bool operator== (const basic_string<__INST>&, const __INST*);
extern template bool operator!= (const basic_string<__INST>&, const basic_string<__INST>&);
extern template bool operator!= (const __INST*, const basic_string<__INST>&);
extern template bool operator!= (const basic_string<__INST>&, const __INST*);
extern template bool operator< (const basic_string<__INST>&, const basic_string<__INST>&);
extern template bool operator< (const __INST*, const basic_string<__INST>&);
extern template bool operator< (const basic_string<__INST>&, const __INST*);
extern template bool operator> (const basic_string<__INST>&, const basic_string<__INST>&);
extern template bool operator> (const __INST*, const basic_string<__INST>&);
extern template bool operator> (const basic_string<__INST>&, const __INST*);
extern template bool operator<= (const basic_string<__INST>&, const basic_string<__INST>&);
extern template bool operator<= (const __INST*, const basic_string<__INST>&);
extern template bool operator<= (const basic_string<__INST>&, const __INST*);
extern template bool operator>= (const basic_string<__INST>&, const basic_string<__INST>&);
extern template bool operator>= (const __INST*, const basic_string<__INST>&);
extern template bool operator>= (const basic_string<__INST>&, const __INST*);
extern template istream& operator>> (istream&, basic_string<__INST>&);
extern template ostream& operator<< (ostream&, const basic_string<__INST>&);
extern template istream& getline (istream&, basic_string<__INST>&, __INST);
