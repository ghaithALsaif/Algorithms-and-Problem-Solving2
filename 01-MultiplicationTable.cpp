#include <iostream>
#include <string>
using namespace std;
void printheader(){
    cout<<"\n\t\t\tMuktipiaction Table Frome 1 to 10\n\n";
    for (int i = 1; i <= 10; i++)
    {
       cout<<"\t"<<i;
    }
    cout<<"\n_____________________________________________________________________________________\n";
    
}

string ColumSperator(int i){
if (i<10)
{
    return " |";
}
else
return"|";
}

void PrintMultiplicationTable(){

        printheader();
         for (int i =1; i <= 10; i++)
         {
            cout<<i<<ColumSperator(i);
            for (int j = 1; j <= 10; j++)
            {
                cout<<"\t"<<j*i;
            }
            cout<<"\n";
            
         }
         
    
}

int main()
{
 PrintMultiplicationTable();


 return 0;    
}







