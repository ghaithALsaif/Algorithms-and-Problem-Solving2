#include <iostream>
#include <string>
#include <cmath>
using namespace std;

enum enPerfectNumber
{PerfectNumber ,NotPerfectNumber };

bool IsPerfectNumber(int number)
{
    int sum = 0;
    for(int i = 1 ; i < number ; i++)
    {
        if(number % i == 0)
        {
            sum += i;
        }
    }
    return sum == number;
}

int ReadPositiveNumber(string message)
{
    int number;
    do
    {
        cout << message;
        cin >> number;
    } while (number <= 0);
    return number;
}
enPerfectNumber CheckPerfectNumber(int number)
{
    if(IsPerfectNumber(number))
    {
        return enPerfectNumber::PerfectNumber;
    }
    else
    {
        return enPerfectNumber::NotPerfectNumber;
    }
}
void PrintPerfectNumbers(int number)
{
    for(int i = 1 ; i <= number ; i++)
    {
        if(CheckPerfectNumber(i) == enPerfectNumber::PerfectNumber)
        {
            cout << i << " is a perfect number" << endl;
        }
        else
        {
            cout << i << " is not a perfect number" << endl;
        }
}
}



int main()
{

    
PrintPerfectNumbers(ReadPositiveNumber("Please enter a positive number: "));


 return 0;    
}











