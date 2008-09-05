%calls functions
num_f = 2;
num_x = 3;

func_names = {'f1', 'f2'};
%f;
%f1(1)

mine = @(x) x^2;
mine1 = @(x) 3*x+5;
funcs={mine, mine1};
x=[1,2,3];
y=[4,3,6];

%f{1}(2)

A=zeros(num_x,num_f);
for j=1:num_f
    for i=1:num_x
        A(i,j) = funcs{j}(x(i));
    end
end

A

def1 = 'x^3';
def2 = 'x+5';

eval(def1, x(2))
