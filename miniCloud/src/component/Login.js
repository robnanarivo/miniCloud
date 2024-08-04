import React from 'react';
import { Button, Checkbox, Form, Input } from 'antd';
import MenuBar from './MenuBar';
import { login, signup } from "../fetcher";
const Login = (props) => {
  const { submit_login, setUsername } = props;
  const onFinish = (values) => {
    console.log('Success:', values);
    if(state.button===1){
      setUsername(values.username)
      login(values.username, values.password).then((res) => {
        if(res==true){
          submit_login();
        }
        
      });
      
    }else{
      signup(values.username, values.password).then((res) => {
        alert("sign up success")
      });
    }
  };
  const state = {
    button: 1
  };

  const onFinishFailed = (errorInfo) => {
    console.log('Failed:', errorInfo);
  };
  return (
    <div>
    <Form
      name="basic"
      labelCol={{
        span: 8,
      }}
      wrapperCol={{
        span: 16,
      }}
      initialValues={{
        remember: true,
      }}
      onFinish={onFinish}
      onFinishFailed={onFinishFailed}
      autoComplete="off"
      className='login-form'
    >
      <Form.Item
        className="login-item"
        label="Username"
        name="username"
        rules={[
          {
            required: true,
            message: 'Please input your username!',
          },
        ]}
      >
        <Input />
      </Form.Item>

      <Form.Item
        className="login-item"

        label="Password"
        name="password"
        rules={[
          {
            required: true,
            message: 'Please input your password!',
          },
        ]}
      >
        <Input.Password />
      </Form.Item>

      <Form.Item
        name="remember"
        valuePropName="checked"
        wrapperCol={{
          offset: 8,
          span: 16,
        }}
      >
        <Checkbox>Remember me</Checkbox>
      </Form.Item>

      <Form.Item
        wrapperCol={{
          offset: 8,
          span: 16,
        }}
      >
        <Button type="primary" htmlType="submit" onClick={() => (state.button = 1)} style={{margin: "10px"}}>
          Log In
        </Button>
        <Button type="primary" htmlType="submit" onClick={() => (state.button = 2)} style={{margin: "10px"}}>
          Sign Up
        </Button>
      </Form.Item>
    </Form>
    </div>
  );
};
export default Login;