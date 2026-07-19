#include <iostream>
#include <string>
#include <cmath>
using namespace std;


int readpositive(string message)
{
    int order;
    do
    {
        cout << message;
        cin >> order;
    } while (order <= 0);
    return order;
}



void printpattern(int order)
{
   int secondaryorder = order;
    for(int i =1 ;i<=order;i++) 
    {
        for(int j=0;j<i;j++)
        {
            cout << i;
        }
        
        cout << endl;
    }


}


int main()
{

printpattern(readpositive("Enter a positive number: "));

    
 return 0;     
}



