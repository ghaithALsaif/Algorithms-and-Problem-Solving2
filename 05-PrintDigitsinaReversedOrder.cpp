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
string reverse(string order)
{
    string reverseorder;
    for(int i = order.length() - 1; i >= 0; i--)
    {
     reverseorder += order[i];   
    }
    return reverseorder;

}

void printdigits(string reverseorder)
{
    for(int i = 0 ; i<reverseorder.length(); i++)
    {
        cout << reverseorder[i] << endl;
    }
}


int main()
{

printdigits(reverse(readpositive("Enter a positive number: ")));

 return 0;    
}











