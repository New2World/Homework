import numpy as np
import random
import torch
import torch.nn as nn
import torch.optim as optim

# fix all random seeds for reproducibility
random_seed = 49876532
random.seed(random_seed)
np.random.seed(random_seed)
torch.random.manual_seed(random_seed)

import matplotlib.pyplot as plt

class Maze:
    def __init__(self, start_location=(0,0), end_location=(7,7)):
        self.__maze = np.asarray([
            [0, 0, 0, 0, 0, 0, 0, 0],
            [0, 0, 0, 0, 0, 0, 0, 0],
            [0, 1, 0, 0, 0, 0, 1, 0],
            [0, 0, 0, 0, 1, 0, 0, 0],
            [0, 0, 0, 0, 1, 0, 0, 0],
            [0, 0, 1, 0, 0, 1, 0, 0],
            [0, 0, 0, 0, 0, 0, 1, 1],
            [0, 0, 0, 0, 1, 0, 0, 0]
        ])
        # Actions: 0:up 1:down 2:left 3:right
        self.__actions = {0: (-1, 0), 1: (1, 0), 2: (0, -1), 3: (0, 1)}
        self.__start_location = start_location
        self.__end_location = end_location
        self.__previous_location = None
        self.__current_location = start_location
        self.__maze[self.__current_location[0],self.__current_location[1]] = 2
        # Define rewards
        self.__step_reward = -1
        self.__wall_reward = -2
        self.__final_reward = 10
        # Trajectory
        self.__trajectory = []

    def __clear_maze(self):
        for i in range(self.__maze.shape[0]):
            for j in range(self.__maze.shape[1]):
                if self.__maze[i,j] == 2:
                    self.__maze[i,j] = 0
    
    def __print_maze(self):
        print("█", end='')
        for col in self.__maze[0]:
            print("█", end='')
        print("█")
        for i,row in enumerate(self.__maze):
            print("█", end='')
            for j,col in enumerate(row):
                if (col == 0):
                    print(' ', end='')
                elif (col == 1):
                    print('█', end ='')
                elif (col == 2):
                    print('O', end='')
            print("█")
        print("█", end='')
        for col in self.__maze[0]:
            print("█", end='')
        print("█")
    
    def __check_valid(self, loc):
        if loc[0] < 0 or loc[1] < 0 or loc[0] >= 8 or loc[1] >= 8 or self.__maze[loc[0],loc[1]] == 1:
            return False
        return True

    def init_maze(self, loc=None):
        self.__clear_maze()
        if loc is None:
            self.__current_location = self.__start_location
        else:
            self.__current_location = loc
        assert self.__maze[self.__current_location[0],self.__current_location[1]]==0
        self.__maze[self.__current_location[0],self.__current_location[1]] = 2
        self.__trajectory.clear()
        self.__trajectory.append(self.__current_location)

    def get_maze_1d(self):
        return np.resize(self.__maze, (1,self.__maze.shape[0]*self.__maze.shape[1]))
    
    def judgement(self):
        return self.__current_location == self.__end_location
    
    def take_action(self, action, trajectory=False):
        gameon = True
        update_location = True
        next_location = tuple([sum(val) for val in zip(self.__current_location,self.__actions[action])])
        if next_location == self.__end_location:
            reward = self.__final_reward
            gameon = False
        elif not self.__check_valid(next_location):
            reward = self.__wall_reward
            update_location = False
        else:
            reward = self.__step_reward
        if update_location:
            self.__previous_location = self.__current_location
            self.__current_location = next_location
            self.__trajectory.append(self.__current_location)
            self.__maze[self.__previous_location[0],self.__previous_location[1]] = 0
            self.__maze[self.__current_location[0],self.__current_location[1]] = 2
        return gameon, reward

    def draw_trajectory(self):
        self.__clear_maze()
        for loc in self.__trajectory:
            self.__maze[loc[0],loc[1]] = 2
        self.__print_maze()

class ReplayBuffer:
    def __init__(self, size):
        self.size = size
        self.buffer = []
    
    def __len__(self):
        return len(self.buffer)
    
    def push(self, buff):
        self.buffer.append(buff)
        if len(self.buffer) > self.size:
            self.buffer = self.buffer[-self.size:]
    
    def sample(self, n):
        indices = np.random.choice(len(self.buffer), n, replace=False)
        state, action, next_state, reward = zip(*[self.buffer[i] for i in indices])
        state = torch.vstack(state)
        action = torch.Tensor(action).type(torch.long)
        next_state = torch.vstack(next_state)
        reward = torch.Tensor(reward).type(torch.float)
        return state, action, next_state, reward

class DQNet(nn.Module):
    def __init__(self):
        super(DQNet, self).__init__()
        self.fc1 = nn.Linear(
            in_features=64,
            out_features=36,
        )
        self.relu1 = nn.ReLU()
        self.fc2 = nn.Linear(
            in_features=36,
            out_features=4,
        )
        nn.init.uniform_(self.fc1.weight)
        nn.init.uniform_(self.fc2.weight)
    
    def forward(self, x):
        x = self.fc1(x)
        x = self.relu1(x)
        x = self.fc2(x)
        return x

class Agent:
    def __init__(self, reward_discount, lr=3e-4):
        self.__model = DQNet()
        self.__target = DQNet()
        self.update_target_net()
        self.__loss_fn = nn.MSELoss()
        self.__optimizer = optim.Adam(self.__model.parameters(), lr=lr)
        self.__reward_discount = reward_discount
    
    def update_target_net(self):
        self.__target.load_state_dict(self.__model.state_dict())
    
    def train(self, replay_buffer, batch_size):
        self.__model.train()
        self.__target.eval()
        total_loss = 0
        maze_tensor, action, next_maze_tensor, reward = replay_buffer.sample(batch_size)
        with torch.no_grad():
            next_outp = self.__target(next_maze_tensor).detach().numpy().squeeze()
            expect_outp = torch.Tensor([next_outp[i].max() for i in range(next_outp.shape[0])]).type(torch.float)
            expect_outp = expect_outp*self.__reward_discount+reward
        self.__optimizer.zero_grad()
        state_outp = self.__model(maze_tensor).gather(dim=1,index=action.unsqueeze(-1)).squeeze()
        loss = self.__loss_fn(state_outp, expect_outp)
        total_loss = loss.item()
        loss.backward()
        self.__optimizer.step()
        return total_loss
    
    def predict(self, state_tensor):
        self.__model.eval()
        with torch.no_grad():
            state_outp = self.__model(state_tensor).detach()
        allowed_state = state_outp.squeeze()
        action = torch.argmax(allowed_state).item()
        return action
    
    def save(self):
        torch.save(self.__model.state_dict())
    
    def load(self, ckpt):
        if isinstance(ckpt, str):
            ckpt = torch.load(ckpt)
        self.__model.load_state_dict(ckpt)

maze = Maze()

max_epoch = 1000
max_step = 100
batch_size = 10
reward_discount = 0.5
explore_threshold = 0.6

replay_buffer = ReplayBuffer(50)
agent = Agent(reward_discount, lr=3e-4)

step_record = []
smth_record = []
loss_record = []

for ep in range(max_epoch):
    gameon = True
    maze.init_maze()
    step_counter = 0
    total_loss = 0
    random_counter = 0
    while gameon and not maze.judgement():
        rnd = random.random()
        maze_tensor = torch.from_numpy(maze.get_maze_1d()).type(torch.float)
        if rnd < explore_threshold:
            random_counter += 1
            action = random.choice([0,1,2,3])
        else:
            action = agent.predict(maze_tensor)
        gameon, reward = maze.take_action(action)
        replay_buffer.push((
            maze_tensor.detach(),
            action,
            torch.from_numpy(maze.get_maze_1d()).type(torch.float),
            reward
        ))
        if len(replay_buffer) >= batch_size:
            total_loss += agent.train(replay_buffer, batch_size)
        step_counter += 1
        if step_counter >= max_step:
            break
    # Periodically update target network
    if (ep+1) % 20 == 0:
        agent.update_target_net()
    # Explore decay
    if (ep+1) % 50 == 0:
        explore_threshold *= 0.90
    # Record & output
    step_record.append(step_counter)
    if ep == 0:
        smth_record.append(step_counter)
    else:
        smth_record.append(smth_record[-1]*0.6+step_counter*0.4)
    loss_record.append(total_loss/step_counter)
    print(f"#{ep+1:4d} - {step_counter:3d} steps - loss: {total_loss/step_counter:.4f} - {random_counter/step_counter*100:.2f}% exploration",end="")
    if maze.judgement():
        print(" - congr")
    else:
        print()

plot_step = 5
xs = np.arange(0,max_epoch,step=plot_step)

fig = plt.figure(figsize=(16,5))

ax1 = plt.subplot(121)
ax1.set_title("Steps")
ax1.plot(xs,step_record[::plot_step],c="r",alpha=0.2,label="real steps")
ax1.plot(xs,smth_record[::plot_step],c="r",label="smoothed")
ax1.legend(loc="upper right")

ax2 = plt.subplot(122)
ax2.set_title("Loss")
ax2.plot(xs,loss_record[::plot_step],c="b")
ax2.set_ylim([0,1e-2])

plt.tight_layout()
plt.show()

step_counter = 0
gameon = True

maze.init_maze()

while gameon and not maze.judgement():
    maze_tensor = torch.from_numpy(maze.get_maze_1d()).type(torch.float)
    action = agent.predict(maze_tensor)
    gameon,_ = maze.take_action(action,trajectory=True)
    step_counter += 1
    if step_counter >= max_step:
        break
maze.draw_trajectory()
print(f"Steps: {step_counter}",end="")
if maze.judgement():
    print(" - congr")
else:
    print(" - sorry")
