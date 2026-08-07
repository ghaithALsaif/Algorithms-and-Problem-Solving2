#include <iostream>
#include <string>
#include <cmath>
#include <cstdlib>
#include <ctime>
using namespace std;

int RandomGenarator(int min, int max)
{
    return rand() % (max - min + 1) + min;
}
int arr[100]; // Assuming a maximum size of 100
int copyArr[100]; // Assuming a maximum size of 100
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
int copyOfArray(int size)
{
    for (int i = 0; i < size; i++)
    {
        copyArr[i] = arr[i];
    }
    return 0;
}
void printArray(int size)
{
    
    fillArrayWithRandomNumbers(size);
    cout << "Array 1 elements: ";
    copyOfArray(size);
    for (int i = 0; i < size; i++)
    {
        cout << arr[i] << " ";
    }
    cout << endl;
    cout << "Array 2 elements: ";
    for (int i = 0; i < size; i++)
    {
        cout << copyArr[i] << " ";
    }
    cout << endl;
}


int main()
{
    srand((unsigned)time(NULL));

printArray(ReadPositiveInteger("Enter the size of the array (1-100): "));
    return 0;
}