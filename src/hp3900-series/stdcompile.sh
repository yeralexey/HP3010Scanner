echo "hp3900 stand-alone compiling process."
echo "This is a fast but insecure way to compile stand-alone application. It may fail,"
echo "If sources compile without problems no message will be shown"
echo "For any question, send mail to Jonathan Bravo Lopez <jkdsoft@gmail.com>"
echo
echo "Compiling..."

gcc hp3900.c -o hp3900 -lusb -lrt -lm -g -ltiff -DSTANDALONE -Wall -Wcast-align -Wcast-qual -Wmissing-declarations -Wmissing-prototypes -Wpointer-arith -Wreturn-type -Wstrict-prototypes -pedantic 

