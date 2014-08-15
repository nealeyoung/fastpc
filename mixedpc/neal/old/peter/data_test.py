import sys

    
class Node:

    
    class Reference:
        def __init__ (self,val,array,location,node):
            self.val = val
            self.array = array
            self.location = location
            self.node = node

        def delete(self):
            self.node.delete()


    def __init__ (self,val,array,location):
        ref= self.Reference(val,array,location,self)
        self.prev = None
        self.next = None
        self.ref = ref
        self.empty = False

    def add(self,val,array,location):
        new = Node(val,array,location)
        self.next = new
        new.prev = self
        return new
        
    def delete(self):
        if self.prev is None and self.next is None:
            self.ref.array[self.ref.location] = None
            self.empty = True
            return
        if self.prev is None:
            self.ref.array[self.ref.location] = None
            self.ref = self.next.ref
            self.next.ref.node = self
            self.next = self.next.next
            if self.next is not None:
                self.next.prev = self
            return
        if self.next is None:
            self.ref.array[self.ref.location] = None
            self.prev.next = None
            return

        self.ref.array[self.ref.location] = None
        self.prev.next = self.next
        self.next.prev = self.prev


    def generate(self):
        if self.empty:
            print "empty"
            return
        yield self
        while self.next is not None:
            yield self.next
            self = self.next
        
    def print_self(self):
        print 'linked list'
        for item in self.generate():
            print item.ref.val
        print ''


def print_array(a):
    print 'array'
    for item in a: 
        if item is not None:
            print item.val
        else:
            print "invalid"
    

my_array = []
start = Node(0,my_array,0)
my_array.append(start.ref)
end = start

for i in range(1,4):
    end = end.add(i,my_array,i)
    my_array.append(end.ref)
    

print_array(my_array)
start.print_self()

my_array[3].delete()


print_array(my_array)
start.print_self()

my_array[0].delete()
start.print_self()

my_array[1].delete()
start.print_self()

my_array[2].delete()
start.print_self()
print_array(my_array)
