#include <iostream>
#include <string>
#include <cmath>
#include <cstdlib>
#include <ctime>
using namespace std;
enum enprime { NOTPRIME, PRIME };
int RandomGenarator(int min, int max)
{
    return rand() % (max - min + 1) + min;
}
int arr[100]; // Assuming a maximum size of 100
int copyprime[100]; // Assuming a maximum size of 100
int ReadPositiveInteger(string message )
{
    int num;
    do
    {
        cout << message;
        cin >> num;
    } while (num < 1 || num > 100);
    return num;
}
int fillArrayWithRandomNumbers( int size)
{
    for (int i = 0; i < size; i++)
    {
        arr[i] = RandomGenarator(1, 100);
    }
    return 0;
}
bool isPrime(int num)
{
    if (num <= 1)
        return false;
    for (int i = 2; i <= sqrt(num); i++)
    {
        if (num % i == 0)
            return false;
    }
    return true;
}
int copyOnlyPrimeNumbers(int size)
{
    int primeCount = 0;
    for (int i = 0; i < size; i++)
    {
        if (isPrime(arr[i]))
        {
            copyprime[primeCount] = arr[i];
            primeCount++;
        }
    }
    return primeCount;
}
   

void printArray(int size)
{
    
    fillArrayWithRandomNumbers(size);
    cout << "Original Array elements: ";
    int primeCount = copyOnlyPrimeNumbers(size);
    for (int i = 0; i < size; i++)
    {
        cout << arr[i] << " ";
    }
    cout << endl;
    cout << "Prime Array elements: ";
    for (int i = 0; i < primeCount; i++)
    {
        cout << copyprime[i] << " ";
    }
    cout << endl;
}


int main()
{
    srand((unsigned)time(NULL));

printArray(ReadPositiveInteger("Enter the size of the array (1-100): "));
    return 0;
}