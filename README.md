# BNF

Parser follows following grammar rules
```
<impl>        ::= <disj> -> <impl>
                | <disj>
<disj>        ::= <conj> <disj'>
<disj'>       ::= or <conj> <disj'>
                | EPS
<conj>        ::= <unary> <conj'>
<conj'>       ::= and <unary> <conj'>
                | EPS
<unary>       ::= ( <impl> )
                | ~ unary
                | @ <var> . ( <impl> )
                | ? <var> . ( <impl> )
                | <pred> (<term_list>)
<term>        ::= <constant>
                | <var>
                | <fn_symbol> (<term_list>)
<term_list>   ::= <term> <term_list'>
<term_list'>  ::= , <term_list>
		| EPS
<name>        ::= <char>
                | <char><name>
<char>        ::= ('A'...'Z'|'a'...'z'|'0'...'9'|''')
<constant>    ::= c<name>
<var>         ::= v<name>
<renamed_var> ::= vu<name>
<fn_symbol>   ::= f<name>
<pred>        ::= p<name>
```
