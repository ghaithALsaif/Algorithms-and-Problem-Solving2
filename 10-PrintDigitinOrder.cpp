#include <iostream>
#include <string>
#include <cmath>
using namespace std;

int ReadFrequency(string message)
{
    int number;
    do
    {
        cout << message;
        cin >> number;
    } while (number < 0);
    return number;
}

string readpositive(string message)
{
    string order;
    do
    {
        cout << message;
        cin >> order;
    } while (stoi(order) <= 0);
    return order;
}


void printNumber(string order)
{
    for(int i = 0; i < order.length(); i++)
    {
        cout << order[i] << endl;
    }
    
    
    
}



int main()
{

printNumber(readpositive("Enter a positive number: "));
 return 0;    
}



