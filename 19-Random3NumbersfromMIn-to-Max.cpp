#include <iostream>
#include <string>
#include <cmath>
#include <cstdlib>
using namespace std;



int numberInput(string message)
{

    int number;
    do
    {
        cout << message;
        cin >> number;
    } while (number < 1 );
    return number;
}

int RandomNumber(int min, int max)
{
    return rand() % (max - min + 1) + min;
}
void PrintRandomNumbers(int min, int max)
{
    cout << "\nRandom numbers: \n" << RandomNumber(min, max) << 
    ",\n " << RandomNumber(min, max) << 
    ",\n " << RandomNumber(min, max) << endl;
}




int main()
{
    srand((unsigned)time(NULL));
PrintRandomNumbers(numberInput("Enter the minimum number: "), numberInput("Enter the maximum number: "));

    
 return 0;
}



