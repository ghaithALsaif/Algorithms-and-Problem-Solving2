#include <iostream>
#include <string>
#include <cmath>
using namespace std;
enum enprimnotprime{prime, notprime};

float ReadNumbers(string massage)
{
    float num;
    
    do
    {
        cout << massage <<endl;
        cin >> num;
    } while (num <= 0);
    return num;
}
enum enprimnotprime IsPrime(int num)
{
    if (num <= 1) return enprimnotprime::notprime;
    
    if (num == 2) return enprimnotprime::prime;

    int limit = round(sqrt(num));
    for (int i = 2; i <= limit; i++) 
   {

        if (num % i == 0) 
        {
            return enprimnotprime::notprime;
        }
    }
    
    
    return enprimnotprime::prime;
}


void printAllPrimeNumbers1toN(int num)
{
    for(int i = 2; i <= num; i++)
    {
        if(IsPrime(i) == enprimnotprime::prime)
        {
            cout << i << " ";
        }
    }

}
int main()
{
printAllPrimeNumbers1toN(ReadNumbers("\nPlease enter a number to print all prime numbers from 1 to N: "));


 return 0;    
}







