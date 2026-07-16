#include <iostream>
#include <string>
#include <cmath>
using namespace std;


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
int SumDigits(string order)
{
    int sum = 0;
    for(int i = 0; i < order.length(); i++)
    {
       sum += order[i] -48;
    }
    return sum;

}

void printdigits(int sum_of_digits)
{
    cout << "\nSum of digits: " << sum_of_digits << endl;
}



int main()
{

printdigits(SumDigits(readpositive("Enter a positive number: ")));

 return 0;    
}











