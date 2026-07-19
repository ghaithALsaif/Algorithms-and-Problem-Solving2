#include <iostream>
#include <string>
#include <cmath>
using namespace std;


/*int readpositive(string message)
{
    int order;
    do
    {
        cout << message;
        cin >> order;
    } while (order <= 0);
    return order;
}*/



void printpattern()
{
  
    for(char i ='A';i<='Z';i++) 
    {
        for(char j='A';j<='Z';j++)
        {
            for(char k='A';k<='Z';k++)
            {
            cout <<i<<j<<k<<endl;
            }   
        }

    }
}
int main()
{

printpattern();

    
 return 0;     
}



