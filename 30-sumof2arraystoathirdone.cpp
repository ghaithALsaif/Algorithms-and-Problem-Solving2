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
int arr1[100]; // Assuming a maximum size of 100
int arr2[100]; // Assuming a maximum size of 100
int sumArray[100]; // Assuming a maximum size of 100
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
int fillfirstArrayWithRandomNumbers( int size)
{
    for (int i = 0; i < size; i++)
    {
        arr1[i] = RandomGenarator(1, 100);
    }
    return 0;
}

int fillsecondArrayWithRandomNumbers(int size)
{
    for (int i = 0; i < size; i++)
    {
        arr2[i] = RandomGenarator(1, 100);
    }
    return 0;
}

int sumarrays(int size)
{
    int sum = 0;
    for (int i = 0; i < size; i++)
    {
        sumArray[i] = arr1[i] + arr2[i];
    }
    return sum;
}
   

void printArray(int size)
{
    fillfirstArrayWithRandomNumbers(size);
    fillsecondArrayWithRandomNumbers(size);
    cout << "Array 1 elements: ";
    for (int i = 0; i < size; i++)
    {
        cout << arr1[i] << " ";
    }
    cout << endl;
    cout << "Array 2 elements: ";
    for (int i = 0; i < size; i++)
    {
        cout << arr2[i] << " ";
    }
    cout << endl;
    sumarrays(size);
    cout << "Sum Array elements: ";
    for (int i = 0; i < size; i++)
    {
        cout << sumArray[i] << " ";
    }
    cout << endl;
}


int main()
{
    srand((unsigned)time(NULL));

printArray(ReadPositiveInteger("Enter the size of the array (1-100): "));
    return 0;
}