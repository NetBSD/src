#source: pr16498a.s
#ld: -shared -T pr16498b.t
#readelf: -l --wide
#target: *-*-linux* *-*-gnu* *-*-nacl*

#...
  TLS .*
#...
[ ]+[0-9]+[ ]+tls_data_init .tbss[ ]*
#pass
